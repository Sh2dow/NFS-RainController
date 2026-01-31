#include "RainFlowMW.h"
#include "../NFSMW_PreFEngHook.h"
#include "../Math.h"
#include "../core.h"
#include "../RainConfigController.h"
#include "../PrecipitationController.h"
#include <windows.h>
#include <cmath>

#include "../Game.h"

namespace
{
    inline uint8_t* Ptr(uintptr_t addr) { return reinterpret_cast<uint8_t*>(addr); }
    inline float* FPtr(uintptr_t addr) { return reinterpret_cast<float*>(addr); }
    inline int* IPtr(uintptr_t addr) { return reinterpret_cast<int*>(addr); }
    inline uint8_t* RainPtr()
    {
        return *reinterpret_cast<uint8_t**>(Game::RainInstancePtr);
    }

    inline float ReadF(uint8_t* base, uintptr_t off)
    {
        return *reinterpret_cast<float*>(base + off);
    }
    inline void WriteF(uint8_t* base, uintptr_t off, float v)
    {
        *reinterpret_cast<float*>(base + off) = v;
    }
    inline int ReadI(uint8_t* base, uintptr_t off)
    {
        return *reinterpret_cast<int*>(base + off);
    }
    inline void WriteI(uint8_t* base, uintptr_t off, int v)
    {
        *reinterpret_cast<int*>(base + off) = v;
    }
    inline void* ReadP(uint8_t* base, uintptr_t off)
    {
        return *reinterpret_cast<void**>(base + off);
    }
    inline void WriteP(uint8_t* base, uintptr_t off, void* v)
    {
        *reinterpret_cast<void**>(base + off) = v;
    }

    static uint8_t* GetActiveView()
    {
        if (!Game::EViewArrayBase || !Game::EViewSize || !Game::EViewArrayCount)
            return nullptr;
        for (uintptr_t i = 0; i < Game::EViewArrayCount; ++i)
        {
            uintptr_t entry = Game::EViewArrayBase + i * Game::EViewSize;
            if (!core::IsReadable(reinterpret_cast<void*>(entry), Game::EViewActiveFlagOffset + 1))
                continue;
            unsigned char active = *reinterpret_cast<unsigned char*>(entry + Game::EViewActiveFlagOffset);
            if (!active)
                continue;
            uint8_t* view = *reinterpret_cast<uint8_t**>(entry);
            if (view && core::IsReadable(view, 0x80))
                return view;
        }
        return nullptr;
    }

    // Helper math
    inline Vec3 Vec3FromPtr(float* p) { return {p[0], p[1], p[2]}; }

    // Helper function pointers (engine)
    using Sub6C1120_t = bool(__thiscall*)(void*, void* eView, void* matrix, void* viewPlat);
    using Sub6C10E0_t = void(__thiscall*)(void*);
    using Sub6D1E30_t = void(__thiscall*)(void*, void* poly);
    using Sub6D1F60_t = void(__thiscall*)(void*);

    using Sub6C8000_t = void(__cdecl*)(void*, void*);
    using Sub6C6CC0_t = void(__cdecl*)(void*, int);
    using Sub6C0F80_t = void(__thiscall*)(void*, void*);

    using Sub6D1E30Fn_t = void(__thiscall*)(void*, void*);
    using Sub6D1F60Fn_t = void(__thiscall*)(void*);

    using Sub6C1120Fn_t = uint8_t(__thiscall*)(void*, void*, void*, void*);
    using Sub6C10E0Fn_t = void(__thiscall*)(void*);
    using Sub6D1E30Fn_t = void(__thiscall*)(void*, void*);
    using Sub6D1F60Fn_t = void(__thiscall*)(void*);

    static auto fn6C1120 = reinterpret_cast<Sub6C1120Fn_t>(0x006C1120);
    static auto fn6C10E0 = reinterpret_cast<Sub6C10E0Fn_t>(0x006C10E0);
    static auto fn6D1E30 = reinterpret_cast<Sub6D1E30Fn_t>(0x006D1E30);
    static auto fn6D1F60 = reinterpret_cast<Sub6D1F60Fn_t>(0x006D1F60);

    // math helpers (2D normalize and 3D normalize)
    static Vec3 Normalize3(const Vec3& v)
    {
        return norm(v);
    }
    static Vec3 Normalize2(const Vec3& v)
    {
        float l = std::sqrt(v.x * v.x + v.y * v.y);
        if (l <= 0.0f)
            return {1.0f, 0.0f, 0.0f};
        float inv = 1.0f / l;
        return {v.x * inv, v.y * inv, 0.0f};
    }

    static float bRandom(float maxVal)
    {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * maxVal;
    }

    static float bSin(unsigned short a)
    {
        float rad = (static_cast<float>(a) * (2.0f * 3.14159265f)) / 65536.0f;
        return sinf(rad);
    }
    static float bCos(unsigned short a)
    {
        float rad = (static_cast<float>(a) * (2.0f * 3.14159265f)) / 65536.0f;
        return cosf(rad);
    }

    static float Sub73C980()
    {
        // Placeholder: use time as input to mimic game noise phase.
        return static_cast<float>(timeGetTime()) * 0.001f;
    }

    static float Sub73C9F0(float a1)
    {
        float v = (Sub73C980() * 0.71428573f + a1 * 0.02f) * 65536.0f;
        int idx = static_cast<int>(v / 360.0f);
        float s = bSin(static_cast<unsigned short>(idx));
        if (s < 0.0f)
            s = 0.0f;
        return 1.0f - fabsf(s);
    }

    static float Sub73CA50(float a1)
    {
        float v = (Sub73C980() * 0.71428573f + a1 * 0.02f) * 65536.0f;
        int idx = static_cast<int>(v / 360.0f);
        float c = bCos(static_cast<unsigned short>(idx));
        float out = c;
        if (out < 0.0f)
        {
            uint8_t online = *reinterpret_cast<uint8_t*>(Game::kOnlineFlag);
            if (!online)
                out = 0.0f;
        }
        return 1.0f - fabsf(out);
    }

    static float Sub7539D0(float x, float y)
    {
        uint8_t online = *reinterpret_cast<uint8_t*>(Game::kOnlineFlag);
        if (!online)
        {
            float v = *FPtr(Game::kWindMod);
            if (v > 0.0f)
                return v;
            if (v < 0.0f)
                return 0.0f;
        }
        float t = Sub73C9F0(x);
        if (online)
            t = t * t;

        void* layer = *reinterpret_cast<void**>(Game::kParamMapLayerRain);
        if (!layer)
            return 0.0f;

        using GetParam_t = void* (__thiscall*)(void*, float, float);
        auto getParam = reinterpret_cast<GetParam_t>(0x0074B5C0); // ParameterMapLayer::GetParameterData
        void* param = getParam(layer, x, y);
        *reinterpret_cast<void**>(Game::kParamDataRain) = param;
        float minVal = 0.0f;
        if (param)
        {
            auto table = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(layer) + 0x10);
            if (table)
            {
                uint32_t off = *reinterpret_cast<uint32_t*>(table + 4);
                minVal = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(param) + off);
            }
        }
        if (t > minVal)
            return 0.0f;
        if (!param)
            return 0.0f;
        auto table = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(layer) + 0x10);
        if (!table)
            return 0.0f;
        uint32_t off = *reinterpret_cast<uint32_t*>(table + 0);
        float* out = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(param) + off);
        return out ? *out : 0.0f;
    }

    static float Sub753AC0(float x, float y)
    {
        float t = Sub73CA50(x);
        void* layer = *reinterpret_cast<void**>(Game::kParamMapLayerClouds);
        if (!layer)
            return *reinterpret_cast<float*>(Game::kCloudBase);

        using GetParam_t = void* (__thiscall*)(void*, float, float);
        auto getParam = reinterpret_cast<GetParam_t>(0x0074B5C0);
        void* param = getParam(layer, x, y);
        *reinterpret_cast<void**>(Game::kParamDataClouds) = param;
        float minVal = 0.0f;
        if (param)
        {
            auto table = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(layer) + 0x10);
            if (table)
            {
                uint32_t off = *reinterpret_cast<uint32_t*>(table + 4);
                minVal = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(param) + off);
            }
        }
        if (t > minVal)
            return *reinterpret_cast<float*>(Game::kCloudBase);
        if (param)
        {
            auto table = *reinterpret_cast<uint8_t**>(reinterpret_cast<uint8_t*>(layer) + 0x10);
            if (table)
            {
                uint32_t off = *reinterpret_cast<uint32_t*>(table + 0);
                float* out = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(param) + off);
                if (out)
                    return *out;
            }
        }
        return 0.0f;
    }

    static void Sub749FB0(uint8_t* rain, float a2, float a3, float a4,
                          float a5, float a6, float a7,
                          float a8, float a9, float a10,
                          float a11, float a12, float a13)
    {
        WriteF(rain, 0x3830, a2);
        WriteF(rain, 0x3834, a3);
        WriteF(rain, 0x3838, a4);
        WriteF(rain, 0x3840, a5);
        WriteF(rain, 0x3844, a6);
        WriteF(rain, 0x3848, a7);
        WriteF(rain, 0x3850, a8);
        WriteF(rain, 0x3854, a9);
        WriteF(rain, 0x3858, a10);
        WriteF(rain, 0x3860, a11);
        WriteF(rain, 0x3864, a12);
        WriteF(rain, 0x3868, a13);
        float dx = a5 - a2;
        float dy = a6 - a3;
        float dz = a7 - a4;
        WriteF(rain, 0x3870, std::sqrt(dx * dx + dy * dy + dz * dz));
    }

    static void Sub73CDB0(uint8_t* out, uint8_t* view)
    {
        float dt = *FPtr(Game::kWorldTimeElapsed);
        float* cam = *reinterpret_cast<float**>(view + 0x40);
        if (!core::IsReadable(cam, sizeof(float) * 132))
        {
            *reinterpret_cast<int*>(out) = 0;
            return;
        }
        float speed = std::sqrt(cam[128] * cam[128] + cam[129] * cam[129] + cam[130] * cam[130]) *
            *FPtr(Game::kOnscreenSpeedMod);

        uint8_t* viewData = *reinterpret_cast<uint8_t**>(view + 0x68);
        if (!viewData)
        {
            *reinterpret_cast<int*>(out) = 0;
            return;
        }
        if (*reinterpret_cast<int*>(viewData + 0x234))
        {
            *reinterpret_cast<int*>(out) = 0;
            return;
        }

        int count = 0;
        if (Game::eCurrentViewMode() < 3)
            count = ReadF(*reinterpret_cast<uint8_t**>(view + 0x68), 0x28C) == 0.0f ? 0 : 20;
        else
            count = ReadF(*reinterpret_cast<uint8_t**>(view + 0x68), 0x28C) == 0.0f ? 0 : 10;

        *reinterpret_cast<int*>(out) = count;
        if (count <= 0)
            return;

        float* ptr = reinterpret_cast<float*>(out + 0x0C);
        for (int i = 0; i < count; ++i, ptr += 7)
        {
            if (*reinterpret_cast<int*>(Game::DripFreeze) == 0)
                ptr[0] -= dt;
            if (ptr[0] > 0.0f)
            {
                float v = *FPtr(Game::kOnscreenDripSpeed) * ptr[3] * dt + ptr[-1];
                ptr[-1] = v;
                if (v > 1.0f || ptr[-2] > 1.0f || ptr[-2] < 0.0f)
                {
                    ptr[0] = 0.0f;
                }
                else
                {
                    Vec3 d = {ptr[-2] - 0.5f, v - 0.1f, 0.0f};
                    d = norm(d);
                    ptr[-2] = d.x * speed + ptr[-2];
                    ptr[-1] = d.y * speed;
                }
                if (speed > *FPtr(Game::kOnscreenDropShapeSpeedChange))
                {
                    int idx = *reinterpret_cast<int*>(&ptr[4]);
                    idx = (idx + 1) % 4;
                    *reinterpret_cast<int*>(&ptr[4]) = idx;
                }
            }
            else
            {
                ptr[0] = ptr[1];
                ptr[-2] = bRandom(1.0f);
                ptr[-1] = bRandom(1.0f);
            }
        }
    }

    static void Sub74A070(uint8_t* rain)
    {
        uint8_t* view = *reinterpret_cast<uint8_t**>(rain + 0x288);
        void* viewPlat = *reinterpret_cast<void**>(view + 0x44);
        if (!viewPlat || viewPlat == (void*)(view + 0x44))
            return;

        void* vtbl = *reinterpret_cast<void**>((uint8_t*)viewPlat - 4);
        if (!vtbl)
            return;

        using GetData_t = void*(__thiscall*)(void*);
        auto getData = *reinterpret_cast<GetData_t*>(reinterpret_cast<uint8_t*>(vtbl) + 0x10);
        void* data = getData((uint8_t*)viewPlat - 4);
        if (!data)
            return;

        float* cam = *reinterpret_cast<float**>(view + 0x40);
        float x = cam[16];
        float y = cam[17];
        float w = cam[20] * 4.0f;
        float h = cam[21] * 4.0f;

        float a[2] = {x - w, y - h};
        float b[2] = {x + w, y + h};
        reinterpret_cast<void(__cdecl*)(float*, float*, float*)>(Game::TunnelCameraRelative)(a, reinterpret_cast<float*>(rain + 0x3810),
                                                                             reinterpret_cast<float*>(rain + 0x3808));
        float v[2] = {*(float*)(rain + 0x3808) - *(float*)(rain + 0x3810),
                      -(*(float*)(rain + 0x3814) - *(float*)(rain + 0x3818))};
        v[0] = v[0];
        v[1] = v[1];
        reinterpret_cast<float*(__cdecl*)(float*, float*)>(Game::Normalize2DAddr)(v, v);
        reinterpret_cast<void(__cdecl*)(float*, float*, float*, float*, float*)>(Game::FindBestFacingEdgeAddr)(
            reinterpret_cast<float*>(rain + 0x3810),
            reinterpret_cast<float*>(rain + 0x3808),
            reinterpret_cast<float*>(rain + 0x3820),
            reinterpret_cast<float*>(rain + 0x3818),
            reinterpret_cast<float*>(rain + 0x244));
    }

    static void Sub74A160(uint8_t* rain)
    {
        uint8_t* view = *reinterpret_cast<uint8_t**>(rain + 0x288);
        void* viewPlat = *reinterpret_cast<void**>(view + 0x44);
        if (!viewPlat || viewPlat == (void*)(view + 0x44))
            return;

        void* vtbl = *reinterpret_cast<void**>((uint8_t*)viewPlat - 4);
        if (!vtbl)
            return;

        using GetData_t = void*(__thiscall*)(void*);
        auto getData = *reinterpret_cast<GetData_t*>(reinterpret_cast<uint8_t*>(vtbl) + 0x10);
        uint8_t* data = reinterpret_cast<uint8_t*>(getData((uint8_t*)viewPlat - 4));
        if (!data)
            return;

        float* cam = *reinterpret_cast<float**>(view + 0x40);
        Vec3 dir = {cam[20], cam[21], 0.0f};
        dir = Normalize2(dir);
        Vec3 a = {*(float*)(rain + 0x3808), *(float*)(rain + 0x380C), 0.0f};
        Vec3 b = {*(float*)(rain + 0x3810), *(float*)(rain + 0x3814), 0.0f};
        Vec3 c = {*(float*)(data + 0x20), *(float*)(data + 0x24), *(float*)(data + 0x28)};
        Vec3 mid = {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, 0.0f};
        Vec3 diff = {mid.x - c.x, mid.y - c.y, 0.0f};
        diff = Normalize2(diff);
        float dotv = dir.x * diff.x + dir.y * diff.y;
        if (dotv <= 0.0f)
        {
            if (*reinterpret_cast<uint8_t**>(view + 0x68))
            {
                float z = *(float*)(data + 0x28);
                float z2 = z + 0.5f;
                reinterpret_cast<void(__cdecl*)(float, float, float, float, float, float, float, float, float, float, float, float)>(Game::TunnelBloom_SetParams)(
                    *(float*)(rain + 0x3818), *(float*)(rain + 0x381C), z2,
                    *(float*)(rain + 0x3820), *(float*)(rain + 0x3824), z2,
                    *(float*)(rain + 0x3818), *(float*)(rain + 0x381C), z,
                    *(float*)(rain + 0x3820), *(float*)(rain + 0x3824), z);
            }
            return;
        }

        if (*reinterpret_cast<uint8_t**>(view + 0x68))
        {
            float z = *(float*)(data + 0x28);
            float z2 = z + 0.5f;
            reinterpret_cast<void(__cdecl*)(float, float, float, float, float, float, float, float, float, float, float, float)>(Game::TunnelBloom_SetParams)(
                *(float*)(rain + 0x3808), *(float*)(rain + 0x380C), z2,
                *(float*)(rain + 0x3810), *(float*)(rain + 0x3814), z2,
                *(float*)(rain + 0x3808), *(float*)(rain + 0x380C), z,
                *(float*)(rain + 0x3810), *(float*)(rain + 0x3814), z);
        }
    }

    static void Sub74A320(uint8_t* rain, uint8_t* poly)
    {
        float r = bRandom(ReadF(rain, 0x3870));
        float r2 = bRandom(1.0f);
        float r3 = bRandom(10.0f);
        Vec3 dest = {ReadF(rain, 0x3840) - ReadF(rain, 0x3830),
                     ReadF(rain, 0x3844) - ReadF(rain, 0x3834),
                     ReadF(rain, 0x3848) - ReadF(rain, 0x3838)};
        dest = Normalize3(dest);

        float v5 = r3 * ReadF(rain, 0x250);
        float v6 = r3 * ReadF(rain, 0x254);
        float v7 = r3 * ReadF(rain, 0x258);

        float x = dest.x * r + ReadF(rain, 0x3830);
        float y = dest.y * r + ReadF(rain, 0x3834);
        float z = dest.z * r + ReadF(rain, 0x3838);

        float* p = reinterpret_cast<float*>(poly + 16 * ReadI(rain, 0x298));
        p[0] = x + v5;
        p[1] = y + v6;
        p[2] = z + v7;
        p[2] -= r2;

        float* p2 = reinterpret_cast<float*>(poly + 16 * ReadI(rain, 0x29C));
        p2[0] = p[0];
        p2[1] = p[1];
        p2[2] = p[2];
    }

    static void Render3D(uint8_t* rain)
    {
        using Render3D_t = void(__thiscall*)(void*);
        auto render3d = reinterpret_cast<Render3D_t>(Game::RainRender3D);
        render3d(rain);
    }

    static void RenderRain(uint8_t* rain)
    {
        using Render_t = void(__thiscall*)(void*);
        auto render = reinterpret_cast<Render_t>(Game::RainRender);
        render(rain);
    }

    static void UpdateRain(uint8_t* rain)
    {
        using Update_t = void(__thiscall*)(void*);
        auto update = reinterpret_cast<Update_t>(Game::RainUpdate);
        update(rain);
    }

    static void SetRainIntensity(uint8_t* rain, float intensity)
    {
        using SetIntensity_t = void(__thiscall*)(void*, float);
        auto setIntensity = reinterpret_cast<SetIntensity_t>(Game::RainSetIntensityAddr);
        setIntensity(rain, intensity);
    }

    static uint8_t IsPaused()
    {
        using IsPaused_t = uint8_t(__thiscall*)(void*);
        auto isPaused = reinterpret_cast<IsPaused_t>(Game::PausedAddr);
        return isPaused(reinterpret_cast<void*>(Game::GAMEFLOWMGR_STATUS_ADDR));
    }

    static int GetViewMode()
    {
        using GetViewMode_t = int(__cdecl*)();
        auto fn = reinterpret_cast<GetViewMode_t>(Game::CurrentViewMode);
        return fn();
    }

    static uint8_t AmIinATunnelSlow(uint8_t* view, int mode)
    {
        using Fn_t = uint8_t(__cdecl*)(void*, int);
        auto fn = reinterpret_cast<Fn_t>(Game::AmIinATunnelSlow);
        return fn(view, mode);
    }
}

namespace RainFlowMW
{
    void EnforceState(bool enable)
    {
        *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR) = enable ? 1 : 0;
        *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR) = enable ? 1 : 0;
        *reinterpret_cast<int*>(Game::RainEnablePtr) = enable ? 1 : 0;
        // *reinterpret_cast<int*>(Game::ParticleSystemEnablePtr) = enable ? 1 : 0;
    }

    void Disable()
    {
        EnforceState(false);
        *reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR) = 0.0f;
        *reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR) = 0.0f;
        *reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR) = 0.0f;

        uint8_t* rain = RainPtr();
        if (!rain || !core::IsReadable(rain, 0x4000))
            return;

        WriteF(rain, 0x28C, 0.0f);
        WriteF(rain, 0x290, *FPtr(Game::kCloudBase));
        WriteF(rain, 0x3694, 0.0f);
        WriteF(rain, 0x3698, 0.0f);
        WriteF(rain, 0x369C, 0.0f);
        WriteF(rain, 0x36A0, 0.0f);
        WriteF(rain, 0x36A4, 0.0f);
        WriteI(rain, 0x280, 0);
    }

    void Tick()
    {
        uint8_t* rain = RainPtr();
        if (!rain)
            return;

        // if (*reinterpret_cast<int*>(Game::GAMEFLOWMGR_STATUS_ADDR) != 6)
        //     return;

        // uint8_t* view = GetActiveView();
        // if (!view)
        //     return;

        // void* viewPlat = *reinterpret_cast<void**>(view + 0x44);
        // if (viewPlat && viewPlat != reinterpret_cast<void*>(view + 0x44))
        // {
        //     WriteP(rain, 0x284, viewPlat);
        //     WriteP(rain, 0x288, view);
        // }
        // else
        // {
        //     WriteP(rain, 0x284, nullptr);
        // }

        // EnforceState(true);

        float rainPct = RainConfigController::precipitationConfig.rainIntensity;
        if (rainPct <= 0.0f)
            rainPct = 1.0f;
        float fogPct = RainConfigController::precipitationConfig.fogIntensity;
        if (fogPct < 0.0f)
            fogPct = 0.0f;

        *reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR) = rainPct;
        *reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR) = fogPct;
        *reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR) = rainPct;
        *reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR) = rainPct;
        *reinterpret_cast<float*>(Game::PRECIP_DRIVEFACTOR_ADDR) = rainPct;

        // Stabilize rate-of-change to avoid flicker.
        *reinterpret_cast<float*>(Game::PRECIP_RAINRATEOFCHANGE_ADDR) = 1.0f;
        *reinterpret_cast<float*>(Game::PRECIP_CLOUDSRATEOFCHANGE_ADDR) = 10.0f;
        *reinterpret_cast<int*>(Game::RoadReflectionStateAddr) = 3;

        // float* cam = *reinterpret_cast<float**>(view + 0x40);

        if (IsPaused())
        {
            Game::g_originalRainRender(rain);
            return;
        }

        // Sub73CDB0(rain, view);

        // Lock intensities to avoid dampness flicker.
        WriteF(rain, 0x28C, rainPct);
        WriteF(rain, 0x290, rainPct);
        // WriteF(rain, 0x36A0, rainPct); // road dampness
        // WriteF(rain, 0x36A4, 1.0f);
        // Clear tunnel/overpass flags that can toggle reflections.
        // WriteI(rain, 0x23C, 0);
        // WriteI(rain, 0x240, 0);
        
        return;
    }
}
