#include "PrecipitationController.h"
#include "RainConfigController.h"
#include <algorithm>
#include <vector>
#include <d3d9.h>
#include <d3dx9.h>

#include "core.h"
#include "Hooking.Patterns.h"
#include "PerlinNoise.h"
#include "Game.h"

static auto& g_precipitationConfig = RainConfigController::precipitationConfig;

namespace
{
    struct RHWVertex
    {
        float x, y, z, rhw;
        DWORD color;
        float u, v;
    };

    struct WorldVertex
    {
        float x, y, z;
        DWORD color;
        float u, v;
    };

    struct LineVertex
    {
        float x, y, z, rhw;
        DWORD color;
    };

    struct DynamicVB
    {
        IDirect3DVertexBuffer9* vb = nullptr;
        UINT capacity = 0;
        DWORD fvf = 0;
        UINT stride = 0;
    };

    static DynamicVB g_rhwQuadVB{nullptr, 0, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, sizeof(RHWVertex)};
    static DynamicVB g_worldQuadVB{nullptr, 0, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, sizeof(WorldVertex)};
    static DynamicVB g_lineVB{nullptr, 0, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, sizeof(LineVertex)};

    static uint32_t g_rngState = 0xA341316C;

    static inline uint32_t NextRandU32()
    {
        uint32_t x = g_rngState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        g_rngState = x;
        return x;
    }

    static inline float RandFloat01()
    {
        return (NextRandU32() & 0x00FFFFFF) / 16777215.0f;
    }

    static inline int RandRangeInt(int minVal, int maxVal)
    {
        if (maxVal <= minVal)
            return minVal;
        const uint32_t span = static_cast<uint32_t>(maxVal - minVal + 1);
        return static_cast<int>(minVal + (NextRandU32() % span));
    }

    static void ApplyNativePresetGlobalsMW()
    {
        const auto& p = g_precipitationConfig.nativePreset;
        static bool loggedPreset = false;
        if (!loggedPreset)
        {
            char buf[512];
            sprintf_s(buf,
                "[WeatherMod] Preset=%s applyPreset=%d RainCrossing=%.3f FallSpeed=%.3f Gravity=%.3f RadiusY=%.3f Damp=%.3f\n",
                g_precipitationConfig.presetName.c_str(),
                g_precipitationConfig.applyPresetGlobals ? 1 : 0,
                p.rainCrossing,
                p.rainFallSpeed,
                p.rainGravity,
                p.rainRadiusY,
                p.baseDampness);
            OutputDebugStringA(buf);
            loggedPreset = true;
        }

        float beforeCross = 0.0f;
        float beforeFall = 0.0f;
        float beforeGrav = 0.0f;
        float beforeDamp = 0.0f;
        if (Game::PRECIP_RAINY_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINY_ADDR), sizeof(float)))
            beforeCross = *reinterpret_cast<float*>(Game::PRECIP_RAINY_ADDR);
        if (Game::PRECIP_RAINZ_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINZ_ADDR), sizeof(float)))
            beforeFall = *reinterpret_cast<float*>(Game::PRECIP_RAINZ_ADDR);
        if (Game::PRECIP_RAINZCONSTANT_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINZCONSTANT_ADDR), sizeof(float)))
            beforeGrav = *reinterpret_cast<float*>(Game::PRECIP_RAINZCONSTANT_ADDR);
        if (Game::PRECIP_BASEDAMPNESS_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_BASEDAMPNESS_ADDR), sizeof(float)))
            beforeDamp = *reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR);

        if (Game::PRECIP_RAINY_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINY_ADDR) = p.rainCrossing;
        if (Game::PRECIP_RAINZ_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINZ_ADDR) = p.rainFallSpeed;
        if (Game::PRECIP_RAINZCONSTANT_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINZCONSTANT_ADDR) = p.rainGravity;
        if (Game::PRECIP_RAINWINDEFF_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINWINDEFF_ADDR) = p.rainWindEff;
        if (Game::PRECIP_RAINRADIUSX_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINRADIUSX_ADDR) = p.rainRadiusX;
        if (Game::PRECIP_RAINRADIUSY_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINRADIUSY_ADDR) = p.rainRadiusY;
        if (Game::PRECIP_RAINRADIUSZ_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINRADIUSZ_ADDR) = p.rainRadiusZ;
        if (Game::PRECIP_BOUNDX_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_BOUNDX_ADDR) = p.boundX;
        if (Game::PRECIP_BOUNDY_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_BOUNDY_ADDR) = p.boundY;
        if (Game::PRECIP_BOUNDZ_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_BOUNDZ_ADDR) = p.boundZ;
        if (Game::PRECIP_AHEADX_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_AHEADX_ADDR) = p.aheadX;
        if (Game::PRECIP_AHEADY_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_AHEADY_ADDR) = p.aheadY;
        if (Game::PRECIP_AHEADZ_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_AHEADZ_ADDR) = p.aheadZ;
        if (Game::PRECIP_DRIVEFACTOR_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_DRIVEFACTOR_ADDR) = p.driveFactor;
        if (Game::PRECIP_RAINRATEOFCHANGE_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINRATEOFCHANGE_ADDR) = p.rainRateOfChange;
        if (Game::PRECIP_CLOUDSRATEOFCHANGE_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_CLOUDSRATEOFCHANGE_ADDR) = p.cloudsRateOfChange;
        if (Game::PRECIP_WINDANG_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_WINDANG_ADDR) = p.windAngle;
        if (Game::PRECIP_SWAYMAX_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_SWAYMAX_ADDR) = p.swayMax;
        if (Game::PRECIP_MAXWINDEFF_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_MAXWINDEFF_ADDR) = p.maxWindEff;
        if (Game::PRECIP_PREVAILINGMULT_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_PREVAILINGMULT_ADDR) = p.prevailingMult;
        if (Game::PRECIP_ONSCREEN_DRIPSPEED_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_ONSCREEN_DRIPSPEED_ADDR) = p.onScreenDripSpeed;
        if (Game::PRECIP_ONSCREEN_SPEEDMOD_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_ONSCREEN_SPEEDMOD_ADDR) = p.onScreenSpeedMod;
        if (Game::PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_ONSCREEN_DROPSHAPESPEEDCHANGE_ADDR) = p.onScreenDropShapeSpeedChange;
        if (Game::PRECIP_BASEDAMPNESS_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR) = p.baseDampness;
        if (Game::PRECIP_RAININTHEHEADLIGHTS_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAININTHEHEADLIGHTS_ADDR) = p.rainInHeadlights;

        static int readbackCountdown = 3;
        if (readbackCountdown > 0)
        {
            --readbackCountdown;
            float afterCross = beforeCross;
            float afterFall = beforeFall;
            float afterGrav = beforeGrav;
            float afterDamp = beforeDamp;
            if (Game::PRECIP_RAINY_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINY_ADDR), sizeof(float)))
                afterCross = *reinterpret_cast<float*>(Game::PRECIP_RAINY_ADDR);
            if (Game::PRECIP_RAINZ_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINZ_ADDR), sizeof(float)))
                afterFall = *reinterpret_cast<float*>(Game::PRECIP_RAINZ_ADDR);
            if (Game::PRECIP_RAINZCONSTANT_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINZCONSTANT_ADDR), sizeof(float)))
                afterGrav = *reinterpret_cast<float*>(Game::PRECIP_RAINZCONSTANT_ADDR);
            if (Game::PRECIP_BASEDAMPNESS_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_BASEDAMPNESS_ADDR), sizeof(float)))
                afterDamp = *reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR);
            char buf[256];
            sprintf_s(buf,
                "[WeatherMod] preset readback cross=%.3f->%.3f fall=%.3f->%.3f grav=%.3f->%.3f damp=%.3f->%.3f\n",
                beforeCross, afterCross, beforeFall, afterFall, beforeGrav, afterGrav, beforeDamp, afterDamp);
            OutputDebugStringA(buf);
        }
    }

    static bool EnsureDynamicVB(IDirect3DDevice9* device, DynamicVB& vb, UINT neededVerts)
    {
        if (!device || neededVerts == 0)
            return false;

        if (vb.vb && vb.capacity >= neededVerts)
            return true;

        if (vb.vb)
        {
            vb.vb->Release();
            vb.vb = nullptr;
            vb.capacity = 0;
        }

        UINT newCapacity = vb.capacity ? vb.capacity * 2 : 1024;
        if (newCapacity < neededVerts)
            newCapacity = neededVerts;

        if (FAILED(device->CreateVertexBuffer(newCapacity * vb.stride,
                                             D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                             vb.fvf,
                                             D3DPOOL_DEFAULT,
                                             &vb.vb,
                                             nullptr)))
        {
            vb.vb = nullptr;
            vb.capacity = 0;
            return false;
        }

        vb.capacity = newCapacity;
        return true;
    }

    template <typename TVertex>
    static void DrawDynamicVB(IDirect3DDevice9* device, DynamicVB& vb, const std::vector<TVertex>& verts,
                              D3DPRIMITIVETYPE primType, UINT primCount)
    {
        if (!device || verts.empty())
            return;

        if (!EnsureDynamicVB(device, vb, static_cast<UINT>(verts.size())))
            return;

        void* dst = nullptr;
        const UINT byteCount = static_cast<UINT>(verts.size()) * vb.stride;
        if (FAILED(vb.vb->Lock(0, byteCount, &dst, D3DLOCK_DISCARD)))
            return;

        std::memcpy(dst, verts.data(), byteCount);
        vb.vb->Unlock();

        device->SetStreamSource(0, vb.vb, 0, vb.stride);
        device->SetFVF(vb.fvf);
        device->DrawPrimitive(primType, 0, primCount);
    }
}

static D3DXMATRIX g_ViewMatrix{};
static D3DXMATRIX g_d3dViewMatrix{};
static D3DXMATRIX g_d3dProjMatrix{};
static bool g_ViewValid = false;
static bool g_d3dViewValid = false;
static bool g_d3dProjValid = false;
static void* g_ActiveViewPtr = nullptr;
static constexpr bool kRainDebug = false;

static inline void RainDebugOut(const char* msg)
{
    (void)msg;
    if (!kRainDebug)
        return;
    OutputDebugStringA(msg);
}

static inline void DebugLogMatrixSourceOnce(const char* tag)
{
    (void)tag;
}

static inline void DebugLogMatrixOnce(const char* tag, const D3DXMATRIX& m)
{
    (void)tag;
    (void)m;
}

static inline void DebugLogMatrixNoGuard(const char* tag, const D3DXMATRIX& m)
{
    (void)tag;
    (void)m;
}

static bool IsIdentityMatrix(const D3DXMATRIX& m)
{
    return m.m[0][0] == 1.0f && m.m[1][1] == 1.0f && m.m[2][2] == 1.0f && m.m[3][3] == 1.0f &&
        m.m[0][1] == 0.0f && m.m[0][2] == 0.0f && m.m[0][3] == 0.0f &&
        m.m[1][0] == 0.0f && m.m[1][2] == 0.0f && m.m[1][3] == 0.0f &&
        m.m[2][0] == 0.0f && m.m[2][1] == 0.0f && m.m[2][3] == 0.0f &&
        m.m[3][0] == 0.0f && m.m[3][1] == 0.0f && m.m[3][2] == 0.0f;
}

static bool IsProjectionLike(const D3DXMATRIX& m)
{
    return m.m[3][3] == 0.0f && m.m[2][3] != 0.0f;
}

static bool UseAltEViewMatrices()
{
    if (detected_game != GameType::MW)
        return false;
    auto* flagPtr = reinterpret_cast<const unsigned char*>(Game::EViewAltMatrixFlagPtr);
    if (!core::IsReadable(const_cast<unsigned char*>(flagPtr), sizeof(unsigned char)))
        return false;
    const unsigned char* inst = *reinterpret_cast<const unsigned char* const*>(flagPtr);
    if (!inst)
        return false;
    if (!core::IsReadable(const_cast<unsigned char*>(inst), 0x51))
        return false;
    return inst[0x50] != 0;
}

static void* NormalizeCameraPtr(void* camPtr)
{
    if (!camPtr)
        return nullptr;
    // Reject common debug fill patterns.
    const uintptr_t rawDbg = reinterpret_cast<uintptr_t>(camPtr);
    if (rawDbg == 0xCDCDCDCDu || rawDbg == 0xDDDDDDDDu || rawDbg == 0xFEEEFEEEu)
        return nullptr;
    const uintptr_t raw = reinterpret_cast<uintptr_t>(camPtr);
    const uintptr_t masks[] = {
        raw,
        raw & 0x7FFFFFFF,
        raw & 0x3FFFFFFF,
        raw & 0x1FFFFFFF,
        raw & 0x0FFFFFFF
    };
    for (uintptr_t m : masks)
    {
        void* p = reinterpret_cast<void*>(m);
        if (core::IsReadable(p, sizeof(void*)))
            return p;
    }
    return camPtr;
}

static inline float GetUpCoord(const D3DXVECTOR3& v)
{
    // MW/CB are Z-up; ProStreet/Undercover are Y-up.
    if (detected_game == GameType::MW || detected_game == GameType::CB)
        return v.z;
    return v.y;
}

static inline void SetUpCoord(D3DXVECTOR3& v, float up)
{
    if (detected_game == GameType::MW || detected_game == GameType::CB)
        v.z = up;
    else
        v.y = up;
}

bool PrecipitationController::IsActive() const
{
    return m_active;
}

void PrecipitationController::UpdateViewMatrix(const D3DXMATRIX& view)
{
    g_ViewMatrix = view;
    g_ViewValid = true;
}

void PrecipitationController::ResetViewMatrix()
{
    g_ViewValid = false;
    g_ViewMatrix = D3DXMATRIX{};
}

bool PrecipitationController::GetViewMatrix(D3DXMATRIX& outView)
{
    if (!g_ViewValid)
        return false;
    outView = g_ViewMatrix;
    return true;
}

void PrecipitationController::UpdateActiveViewPtr(void* viewPtr)
{
    if (viewPtr && core::IsReadable(viewPtr, Game::EViewMatrixOffset1 + sizeof(D3DXMATRIX)))
        g_ActiveViewPtr = viewPtr;
}

void PrecipitationController::UpdateD3DTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* mat)
{
    if (!mat)
        return;

    const D3DXMATRIX& src = *reinterpret_cast<const D3DXMATRIX*>(mat);
    if (state == D3DTS_VIEW)
    {
        g_d3dViewMatrix = src;
        g_d3dViewValid = true;
    }
    else if (state == D3DTS_PROJECTION)
    {
        g_d3dProjMatrix = src;
        g_d3dProjValid = true;
    }
}

bool PrecipitationController::GetD3DViewProj(D3DXMATRIX& view, D3DXMATRIX& proj)
{
    if (!g_d3dViewValid || !g_d3dProjValid)
        return false;
    view = g_d3dViewMatrix;
    proj = g_d3dProjMatrix;
    return true;
}

bool PrecipitationController::GetD3DProj(D3DXMATRIX& proj)
{
    if (!g_d3dProjValid)
        return false;
    proj = g_d3dProjMatrix;
    return true;
}

static bool GetRainMatrices(D3DXMATRIX& outView, D3DXMATRIX& outProj)
{
    auto* rain = *reinterpret_cast<void**>(Game::RainInstancePtr);
    if (!rain ||
        !core::IsReadable(rain, Game::RainProjMatrixOffset + sizeof(D3DXMATRIX)))
        return false;

    D3DXMATRIX viewRaw = *reinterpret_cast<D3DXMATRIX*>(
        reinterpret_cast<uintptr_t>(rain) + Game::RainViewMatrixOffset);
    D3DXMATRIX projRaw = *reinterpret_cast<D3DXMATRIX*>(
        reinterpret_cast<uintptr_t>(rain) + Game::RainProjMatrixOffset);

    switch (g_precipitationConfig.rainMatrixMode)
    {
    case 1: // swap
        outView = projRaw;
        outProj = viewRaw;
        break;
    case 2: // transpose
        outView = viewRaw;
        outProj = projRaw;
        D3DXMatrixTranspose(&outView, &outView);
        D3DXMatrixTranspose(&outProj, &outProj);
        break;
    case 3: // swap + transpose
        outView = projRaw;
        outProj = viewRaw;
        D3DXMatrixTranspose(&outView, &outView);
        D3DXMatrixTranspose(&outProj, &outProj);
        break;
    case 0: // raw
    default:
        outView = viewRaw;
        outProj = projRaw;
        break;
    }

    return true;
}

static bool GetViewFromEViewCamera(void* viewPtr, D3DXMATRIX& outView)
{
    if (!viewPtr ||
        !core::IsReadable(viewPtr, Game::EViewCameraParamsOffset + sizeof(void*)))
        return false;
    void* cam = *reinterpret_cast<void**>(
        reinterpret_cast<uintptr_t>(viewPtr) + Game::EViewCameraParamsOffset);
    if (!cam)
        return false;
    auto* mat = reinterpret_cast<D3DXMATRIX*>(reinterpret_cast<uintptr_t>(cam) + 0x50);
    if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
        return false;
    outView = *mat;
    return !IsIdentityMatrix(outView);
}

static bool GetEViewMatrix(D3DXMATRIX& outView)
{
    void* view = g_ActiveViewPtr;
    if (!view)
        view = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
    if (!view || !core::IsReadable(view, sizeof(D3DXMATRIX)))
        return false;
    const uintptr_t offset = UseAltEViewMatrices()
        ? Game::EViewMatrixOffset2
        : Game::EViewMatrixOffset0;
    auto* mat = reinterpret_cast<D3DXMATRIX*>(reinterpret_cast<uintptr_t>(view) + offset);
    if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
        return false;
    outView = *mat;
    return true;
}

static bool GetProjFromEViewMatrix(void* viewPtr, D3DXMATRIX& outProj)
{
    if (!viewPtr || !core::IsReadable(viewPtr, sizeof(D3DXMATRIX)))
        return false;
    const uintptr_t offset = UseAltEViewMatrices()
        ? Game::EViewMatrixOffset3
        : Game::EViewMatrixOffset1;
    auto* mat = reinterpret_cast<D3DXMATRIX*>(reinterpret_cast<uintptr_t>(viewPtr) + offset);
    if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
        return false;
    outProj = *mat;
    return true;
}

static bool GetViewFromCameraPtr(void* cameraPtr, D3DXMATRIX& outView)
{
    if (!cameraPtr || (reinterpret_cast<uintptr_t>(cameraPtr) & 0x3) != 0)
        return false;
    if (!core::IsReadable(cameraPtr, sizeof(D3DXMATRIX)))
        return false;

    __try
    {
        auto* camMat = reinterpret_cast<D3DXMATRIX*>(cameraPtr);
        D3DXMATRIX inv{};
        if (!D3DXMatrixInverse(&inv, nullptr, camMat))
            return false;
        outView = inv;
        return !IsIdentityMatrix(outView);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

static bool IsLikelyPointer(void* p)
{
    if (!p)
        return false;

    struct CacheEntry
    {
        void* ptr;
        DWORD time;
        bool ok;
    };

    static CacheEntry cache[4] = {};
    const DWORD now = core::CurrentTime;

    for (auto& entry : cache)
    {
        if (entry.ptr == p && entry.time == now)
            return entry.ok;
    }

    MEMORY_BASIC_INFORMATION mbi;
    bool ok = VirtualQuery(p, &mbi, sizeof(mbi)) != 0 &&
        mbi.State == MEM_COMMIT &&
        (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) == 0;

    for (auto& entry : cache)
    {
        if (entry.time != now)
        {
            entry.ptr = p;
            entry.time = now;
            entry.ok = ok;
            return ok;
        }
    }

    cache[0] = {p, now, ok};
    return ok;
}

static bool GetCameraMatrixFromPtr(void* cameraPtr, D3DXMATRIX& outMat)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    const uintptr_t base = reinterpret_cast<uintptr_t>(cameraPtr);
    constexpr uintptr_t offsets[] = {0x0, 0x10, 0x20, 0x30, 0x40};
    for (uintptr_t off : offsets)
    {
        auto* mat = reinterpret_cast<D3DXMATRIX*>(base + off);
        if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
            continue;
        outMat = *mat;
        if (!IsIdentityMatrix(outMat))
            return true;
    }
    return false;
}

static bool GetViewFromCameraParams(void* cameraPtr, D3DXMATRIX& outView)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    const uintptr_t base = reinterpret_cast<uintptr_t>(cameraPtr);
    const auto readVec3 = [&](uintptr_t off, D3DXVECTOR3& v) -> bool
    {
        auto* p = reinterpret_cast<const float*>(base + off);
        if (!core::IsReadable(const_cast<float*>(p), sizeof(float) * 3))
            return false;
        v = D3DXVECTOR3(p[0], p[1], p[2]);
        return _finite(v.x) && _finite(v.y) && _finite(v.z);
    };

    D3DXVECTOR3 right{}, up{}, back{}, pos{};
    if (!readVec3(0x00, right) ||
        !readVec3(0x10, up) ||
        !readVec3(0x20, back) ||
        !readVec3(Game::CameraPositionOffset, pos))
        return false;

    if (D3DXVec3LengthSq(&right) < 0.0001f ||
        D3DXVec3LengthSq(&up) < 0.0001f ||
        D3DXVec3LengthSq(&back) < 0.0001f)
        return false;

    // Build row-basis view for D3DX (right/up/back in rows).
    D3DXMatrixIdentity(&outView);
    outView._11 = right.x; outView._12 = right.y; outView._13 = right.z;
    outView._21 = up.x;    outView._22 = up.y;    outView._23 = up.z;
    outView._31 = back.x;  outView._32 = back.y;  outView._33 = back.z;
    outView._41 = -D3DXVec3Dot(&right, &pos);
    outView._42 = -D3DXVec3Dot(&up, &pos);
    outView._43 = -D3DXVec3Dot(&back, &pos);

    return !IsIdentityMatrix(outView);
}

static bool GetProjFromEView(void* eViewPtr, const D3DVIEWPORT9& vp, D3DXMATRIX& outProj)
{
    static bool logged = false;
    if (!eViewPtr || !core::IsReadable(eViewPtr, 0x20))
        return false;

    auto* fov = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(eViewPtr) + 0x1C);
    auto* nz = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(eViewPtr) + 0x10);
    auto* fz = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(eViewPtr) + 0x14);
    if (!core::IsReadable(fov, sizeof(float)) ||
        !core::IsReadable(nz, sizeof(float)) ||
        !core::IsReadable(fz, sizeof(float)))
        return false;

    float fovDeg = *fov;
    float nearZ = *nz;
    float farZ = *fz;
    if (!logged && kRainDebug)
    {
        char buf[256];
        sprintf_s(buf, "[RainDebug] eView proj params fov=%.3f near=%.3f far=%.3f\n", fovDeg, nearZ, farZ);
        RainDebugOut(buf);
        logged = true;
    }
    if (!logged)
        logged = true;
    if (fovDeg <= 0.1f || fovDeg > 179.0f || nearZ <= 0.0f || farZ <= nearZ)
        return false;
    float aspect = static_cast<float>(vp.Width) / static_cast<float>(vp.Height);
    D3DXMatrixPerspectiveFovLH(&outProj, D3DXToRadian(fovDeg), aspect, nearZ, farZ);
    return true;
}

static bool GetProjFromCameraParams(void* cameraPtr, const D3DVIEWPORT9& vp, D3DXMATRIX& outProj)
{
    static bool logged = false;
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    const uintptr_t base = reinterpret_cast<uintptr_t>(cameraPtr);
    auto* nearZp = reinterpret_cast<const float*>(base + 0xBC);
    auto* farZp = reinterpret_cast<const float*>(base + 0xC0);
    auto* fovp = reinterpret_cast<const unsigned short*>(base + 0xC4);
    if (!core::IsReadable(const_cast<float*>(nearZp), sizeof(float)) ||
        !core::IsReadable(const_cast<float*>(farZp), sizeof(float)) ||
        !core::IsReadable(const_cast<unsigned short*>(fovp), sizeof(unsigned short)))
        return false;

    float nearZ = *nearZp;
    float farZ = *farZp;
    unsigned short fovRaw = *fovp;

    // MW uses ushort angle units for bSin/bCos: 0..65535 -> 0..2pi
    float fovRad = static_cast<float>(fovRaw) * (D3DX_PI / 32768.0f);
    float fovDeg = fovRad * (180.0f / D3DX_PI);

    if (!logged && kRainDebug)
    {
        char buf[256];
        sprintf_s(buf, "[RainDebug] camera params fovRaw=%u fovDeg=%.3f fovRad=%.5f near=%.3f far=%.3f\n",
                  fovRaw, fovDeg, fovRad, nearZ, farZ);
        RainDebugOut(buf);
        logged = true;
    }
    if (!logged)
        logged = true;

    if (fovDeg <= 0.1f || fovDeg > 179.0f || nearZ <= 0.0f || farZ <= nearZ)
        return false;

    float aspect = static_cast<float>(vp.Width) / static_cast<float>(vp.Height);
    D3DXMatrixPerspectiveFovLH(&outProj, fovRad, aspect, nearZ, farZ);
    return true;
}

static bool GetViewFromCameraStruct(void* cameraPtr, D3DXMATRIX& outView)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    // Try CameraParams.Matrix at 0x0 (CameraParams) and at 0xD4 (Camera)
    constexpr uintptr_t kCameraParamOffsets[] = {0x0, 0xD4};
    for (uintptr_t off : kCameraParamOffsets)
    {
        auto* mat = reinterpret_cast<D3DXMATRIX*>(reinterpret_cast<uintptr_t>(cameraPtr) + off);
        if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
            continue;

        D3DXMATRIX m = *mat;
        if (!IsIdentityMatrix(m) && !IsProjectionLike(m))
        {
            outView = m;
            return true;
        }

        D3DXMATRIX inv{};
        if (D3DXMatrixInverse(&inv, nullptr, &m))
        {
            if (!IsIdentityMatrix(inv))
            {
                outView = inv;
                return true;
            }
        }
    }

    return false;
}

static bool GetCameraBasisFromStruct(void* cameraPtr, D3DXVECTOR3& outRight, D3DXVECTOR3& outUp,
                                     D3DXVECTOR3& outForward)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;
    auto* mat = reinterpret_cast<D3DXMATRIX*>(cameraPtr);
    if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
        return false;
    D3DXMATRIX m = *mat;
    outRight = D3DXVECTOR3(m._11, m._12, m._13);
    outUp = D3DXVECTOR3(m._21, m._22, m._23);
    outForward = D3DXVECTOR3(m._31, m._32, m._33);
    float len2 = outForward.x * outForward.x + outForward.y * outForward.y + outForward.z * outForward.z;
    if (len2 <= 1e-6f)
        return false;
    D3DXVec3Normalize(&outForward, &outForward);
    return true;
}

static bool GetCameraBasisFromCamParams(void* camParams, D3DXVECTOR3& outRight, D3DXVECTOR3& outUp,
                                        D3DXVECTOR3& outForward)
{
    if (!camParams || !IsLikelyPointer(camParams))
        return false;
    auto* mat = reinterpret_cast<D3DXMATRIX*>(reinterpret_cast<uintptr_t>(camParams) + 0x50);
    if (!core::IsReadable(mat, sizeof(D3DXMATRIX)))
        return false;
    D3DXMATRIX m = *mat;
    outRight = D3DXVECTOR3(m._11, m._12, m._13);
    outUp = D3DXVECTOR3(m._21, m._22, m._23);
    outForward = D3DXVECTOR3(m._31, m._32, m._33);
    float len2 = outForward.x * outForward.x + outForward.y * outForward.y + outForward.z * outForward.z;
    if (len2 <= 1e-6f)
        return false;
    D3DXVec3Normalize(&outForward, &outForward);
    return true;
}

static bool ProjectWithViewProj(const D3DXVECTOR3& world, const D3DVIEWPORT9& vp,
                                const D3DXMATRIX& viewProj, D3DXVECTOR3& outScreen,
                                bool transposeVP)
{
    // MW camera matrices can be column-basis; optionally transpose view-proj.
    D3DXMATRIX vpUse = viewProj;
    if (transposeVP)
        D3DXMatrixTranspose(&vpUse, &vpUse);
    D3DXVECTOR4 v(world.x, world.y, world.z, 1.0f);
    D3DXVECTOR4 clip{};
    D3DXVec4Transform(&clip, &v, &vpUse);
    if (clip.w == 0.0f)
        return false;
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;
    float ndcZ = clip.z / clip.w;
    outScreen.x = vp.X + (ndcX + 1.0f) * 0.5f * vp.Width;
    outScreen.y = vp.Y + (1.0f - ndcY) * 0.5f * vp.Height;
    outScreen.z = (ndcZ + 1.0f) * 0.5f;
    return true;
}

static void DebugLogProjectOnce(const char* tag, const D3DVIEWPORT9& vp, const D3DXMATRIX& viewProj,
                                const D3DXVECTOR3& world, const D3DXVECTOR3& screen)
{
    if (!kRainDebug)
        return;
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    char buf[256];
    sprintf_s(buf, "[RainDebug] %s world=(%.2f,%.2f,%.2f) screen=(%.2f,%.2f,%.2f)\n",
              tag, world.x, world.y, world.z, screen.x, screen.y, screen.z);
    RainDebugOut(buf);
}

static void* GetCameraPtrFromEView()
{
    if (!kRainDebug)
    {
        void* view = g_ActiveViewPtr;
        if (!view)
            view = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
        if (!view)
            return nullptr;
        if (!core::IsReadable(view, Game::EViewCameraParamsOffset + sizeof(void*)))
            return nullptr;
        void* camParams = *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(view) + Game::EViewCameraParamsOffset);
        if (!camParams)
            return nullptr;
        void* camFallback = *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(view) + Game::EViewCameraOffset);
        void* cam = camParams ? camParams : camFallback;
        void* camNorm = NormalizeCameraPtr(cam);
        if (core::IsReadable(camNorm, sizeof(void*)))
            return camNorm;
        return cam;
    }
    static bool logged = false;
    void* view = g_ActiveViewPtr;
    if (!view)
        view = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
    if (!view)
        return nullptr;
    if (!core::IsReadable(view, Game::EViewCameraParamsOffset + sizeof(void*)))
        return nullptr;
    void* camParams = *reinterpret_cast<void**>(
        reinterpret_cast<uintptr_t>(view) + Game::EViewCameraParamsOffset);
    if (!camParams)
        return nullptr;
    void* camFallback = *reinterpret_cast<void**>(
        reinterpret_cast<uintptr_t>(view) + Game::EViewCameraOffset);
    void* cam = camParams ? camParams : camFallback;
    void* camNorm = NormalizeCameraPtr(cam);
    if (!logged)
    {
        char buf[256];
        sprintf_s(buf,
                  "[RainDebug] eViewPtr=0x%08X camParams=0x%08X pCamera=0x%08X pCameraNorm=0x%08X\n",
                  (unsigned)(uintptr_t)view, (unsigned)(uintptr_t)camParams,
                  (unsigned)(uintptr_t)camFallback, (unsigned)(uintptr_t)camNorm);
        RainDebugOut(buf);
        logged = true;
    }
    if (core::IsReadable(camNorm, sizeof(void*)))
        return camNorm;
    return cam;
}

static void* GetCameraParamsFromEView()
{
    void* view = g_ActiveViewPtr;
    if (!view)
        view = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
    if (!view)
        return nullptr;
    void* viewObj = view;
    if (!core::IsReadable(viewObj, Game::EViewCameraParamsOffset + sizeof(void*)))
    {
        if (core::IsReadable(viewObj, sizeof(void*)))
        {
            void* indirect = *reinterpret_cast<void**>(viewObj);
            if (indirect &&
                core::IsReadable(indirect, Game::EViewCameraParamsOffset + sizeof(void*)))
                viewObj = indirect;
        }
    }
    if (!core::IsReadable(viewObj, Game::EViewCameraParamsOffset + sizeof(void*)))
        return nullptr;
    return *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(viewObj) +
                                     Game::EViewCameraParamsOffset);
}

static void GetFrameViewProj(const D3DVIEWPORT9& viewport, D3DXMATRIX& outView, D3DXMATRIX& outProj,
                             bool& outHasProj, bool& outLocked)
{
    static DWORD cachedTime = 0;
    static D3DXMATRIX cachedView{};
    static D3DXMATRIX cachedProj{};
    static bool cachedHasProj = false;
    static bool cachedLocked = false;

    if (cachedTime == core::CurrentTime)
    {
        outView = cachedView;
        outProj = cachedProj;
        outHasProj = cachedHasProj;
        outLocked = cachedLocked;
        return;
    }

    D3DXMATRIX view{};
    D3DXMATRIX proj{};
    bool hasProj = false;
    bool locked = false;
    IDirect3DDevice9* device = PrecipitationController::Get()->m_device;

    if (detected_game == GameType::MW)
    {
        void* viewPtr = g_ActiveViewPtr;
        if (!viewPtr)
            viewPtr = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
        if (viewPtr && GetViewFromEViewCamera(viewPtr, view) && GetProjFromEViewMatrix(viewPtr, proj))
        {
            hasProj = true;
            locked = true;
        }
    }

    if (!locked &&
        !GetEViewMatrix(view) &&
        !(g_precipitationConfig.useLookAtMatrix && PrecipitationController::GetViewMatrix(view)) &&
        device)
    {
        device->GetTransform(D3DTS_VIEW, &view);
        DebugLogMatrixSourceOnce("D3DTS_VIEW fallback");
        DebugLogMatrixOnce("D3DTS_VIEW", view);
    }

    if (!hasProj && !locked)
    {
        void* viewPtr = g_ActiveViewPtr;
        if (!viewPtr)
            viewPtr = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
        void* camPtr = PrecipitationController::Get()->m_cameraPtr;
        if (!camPtr)
            camPtr = GetCameraPtrFromEView();
        void* camPtrNorm = NormalizeCameraPtr(camPtr);
        if (!GetProjFromEView(viewPtr, viewport, proj) &&
            !GetProjFromCameraParams(camPtrNorm, viewport, proj))
        {
            if (device && FAILED(device->GetTransform(D3DTS_PROJECTION, &proj)))
            {
                float aspect = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
                D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(60.0f), aspect, 1.0f, 500.0f);
                DebugLogMatrixSourceOnce("Render3D fallback proj");
                DebugLogMatrixNoGuard("Render3D fallback proj", proj);
            }
        }
    }

    cachedTime = core::CurrentTime;
    cachedView = view;
    cachedProj = proj;
    cachedHasProj = hasProj;
    cachedLocked = locked;

    outView = view;
    outProj = proj;
    outHasProj = hasProj;
    outLocked = locked;
}

static void DebugLogCameraPtrOnce(const char* tag, void* camPtr)
{
    if (!kRainDebug)
        return;
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    char buf[256];
    sprintf_s(buf, "[RainDebug] %s cameraPtr=0x%08X\n", tag, (unsigned)(uintptr_t)camPtr);
    RainDebugOut(buf);
}

static void DebugLogCameraPtrDetailsOnce(void* camPtr)
{
    if (!kRainDebug)
        return;
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(camPtr, &mbi, sizeof(mbi)))
    {
        char buf[256];
        sprintf_s(buf, "[RainDebug] cameraPtr mbi Base=0x%08X Protect=0x%X State=0x%X\n",
                  (unsigned)(uintptr_t)mbi.BaseAddress, (unsigned)mbi.Protect, (unsigned)mbi.State);
        RainDebugOut(buf);
    }
}

static void DebugLogCameraPtrDetailsOnce2(const char* tag, void* camPtr)
{
    if (!kRainDebug)
        return;
    static bool logged = false;
    if (logged)
        return;
    logged = true;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(camPtr, &mbi, sizeof(mbi)))
    {
        char buf[256];
        sprintf_s(buf, "[RainDebug] %s ptr=0x%08X Base=0x%08X Protect=0x%X State=0x%X\n",
                  tag, (unsigned)(uintptr_t)camPtr, (unsigned)(uintptr_t)mbi.BaseAddress,
                  (unsigned)mbi.Protect, (unsigned)mbi.State);
        RainDebugOut(buf);
    }
}

void PrecipitationController::DebugEVIEWListPtr()
{
    if (detected_game != GameType::PS && detected_game != GameType::UC)
    {
        RainDebugOut("[DebugEVIEWListPtr] EVIEW scan not supported for this game.\n");
        return;
    }

    auto match = hook::pattern(
        (detected_game == GameType::PS)
            ? "A1 20 BE A8 00 50 E8 ?? ??"
            : "D9 C9 D9 5C 24 1C D9 44 24 1C DE D9 DF E0 F6 C4 41 75");
    // UC pattern: unique fld/fstp/fcompp/fnstsw/test/loop combo

    if (match.empty())
    {
        RainDebugOut("[DebugEVIEWListPtr] ❌ Pattern not found.\n");
        return;
    }

    // char logBuf[128];
    // sprintf_s(logBuf, "[DebugEVIEWListPtr] Matches found: %zu\n", match.size());
    // RainDebugOut(logBuf);

    if (detected_game == GameType::PS)
    {
        uintptr_t ptrAddr = *match.get(0).get<uintptr_t>(1); // +1 skips A1 opcode
        g_EVIEW_LIST_PTR = reinterpret_cast<EViewNode**>(ptrAddr);
    }
    else if (detected_game == GameType::UC)
    {
        uintptr_t addr = reinterpret_cast<uintptr_t>(match.get_first());
        uintptr_t targetAddr = *reinterpret_cast<uintptr_t*>(addr + 1);
        g_EVIEW_LIST_PTR = reinterpret_cast<EViewNode**>(targetAddr);
    }

    char buf[256];
    sprintf_s(buf, "[DebugEVIEWListPtr] ✅ g_EVIEW_LIST_PTR = 0x%08X -> 0x%08X\n",
              (uintptr_t)g_EVIEW_LIST_PTR, (g_EVIEW_LIST_PTR ? (uintptr_t)*g_EVIEW_LIST_PTR : 0));
    RainDebugOut(buf);
}

bool isLikelyValidPtr(void* p)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof(mbi)))
    {
        DWORD protect = mbi.Protect;

        // Only treat these as safe to read
        bool readable = (protect & PAGE_READONLY) ||
            (protect & PAGE_READWRITE) ||
            (protect & PAGE_EXECUTE_READ) ||
            (protect & PAGE_EXECUTE_READWRITE);

        // char buf[256];
        // sprintf_s(buf, "[isLikelyValidPtr] ✅ EVIEW_PTR state = 0x%X, protect = 0x%X, base = 0x%p\n",
        //           mbi.State, mbi.Protect, mbi.BaseAddress);
        // RainDebugOut(buf);

        if (mbi.State != MEM_COMMIT || !readable)
        {
            RainDebugOut("[isLikelyValidPtr] 🚫 EVIEW ptr is NOT readable.\n");
            return false;
        }

        return true;
    }

    RainDebugOut("[isLikelyValidPtr] ❌ VirtualQuery failed — invalid pointer.\n");
    return false;
}

// Used by rain renderer or world logic
D3DXVECTOR3 PrecipitationController::GetCameraPositionSafe()
{
    if (detected_game == GameType::MW)
    {
        void* viewPtr = g_ActiveViewPtr;
        if (!viewPtr)
            viewPtr = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
        static bool loggedViewPtrOnce = false;
        if (!loggedViewPtrOnce)
        {
            char buf[256];
            sprintf_s(buf, "[GetCameraPositionSafe] viewPtr=0x%08X\n", (unsigned)(uintptr_t)viewPtr);
            RainDebugOut(buf);
            loggedViewPtrOnce = true;
        }
        if (viewPtr && core::IsReadable(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(viewPtr) + 0x40),
                                        sizeof(D3DXMATRIX)))
        {
            D3DXMATRIX view{};
            std::memcpy(&view, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(viewPtr) + 0x40),
                        sizeof(D3DXMATRIX));
            D3DXMATRIX invView{};
            if (D3DXMatrixInverse(&invView, nullptr, &view))
                return D3DXVECTOR3(invView._41, invView._42, invView._43);
        }
        else if (!loggedViewPtrOnce)
        {
            RainDebugOut("[GetCameraPositionSafe] viewPtr unreadable\n");
            loggedViewPtrOnce = true;
        }
        if (m_device)
        {
            D3DXMATRIX view{};
            if (SUCCEEDED(m_device->GetTransform(D3DTS_VIEW, &view)))
            {
                D3DXMATRIX invView{};
                if (D3DXMatrixInverse(&invView, nullptr, &view))
                    return D3DXVECTOR3(invView._41, invView._42, invView._43);
            }
        }

        uintptr_t base = Game::EViewArrayBase;
        D3DXVECTOR3 fallbackPos(0, 0, 0);
        bool hasFallback = false;
        static bool loggedPtr = false;
        for (size_t i = 0; i < Game::EViewArrayCount; ++i)
        {
            auto* view = reinterpret_cast<void*>(base + i * Game::EViewSize);
            if (!view || !core::IsReadable(view, Game::EViewCameraOffset + sizeof(void*)))
                continue;

            auto active = *reinterpret_cast<unsigned char*>(
                reinterpret_cast<uintptr_t>(view) + Game::EViewActiveFlagOffset);
            auto* camera = *reinterpret_cast<void**>(
                reinterpret_cast<uintptr_t>(view) + Game::EViewCameraOffset);
            if (reinterpret_cast<uintptr_t>(camera) & 0x80000000u)
                camera = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(camera) & 0x7FFFFFFFu);
            if (!loggedPtr)
            {
                char buf[256];
                sprintf_s(buf, "[RainDebug] eView entry=0x%08X active=%u camera=0x%08X\n",
                          (unsigned)(uintptr_t)view, (unsigned)active,
                          (unsigned)(uintptr_t)camera);
                RainDebugOut(buf);
                loggedPtr = true;
            }
            if (!camera)
                continue;

            D3DXVECTOR3 candidate(0, 0, 0);
            bool hasCandidate = false;

            if (core::IsReadable(camera, Game::CameraMatrixV3Offset + sizeof(float) * 16))
            {
                auto* camMat = reinterpret_cast<D3DXMATRIX*>(camera);
                D3DXMATRIX invView{};
                if (D3DXMatrixInverse(&invView, nullptr, camMat))
                {
                    candidate = D3DXVECTOR3(invView._41, invView._42, invView._43);
                    hasCandidate = (candidate != D3DXVECTOR3(0, 0, 0));
                }
            }

            if (!hasCandidate &&
                core::IsReadable(camera, Game::CameraPositionOffset + sizeof(float) * 3))
            {
                auto* pos = reinterpret_cast<float*>(
                    reinterpret_cast<uintptr_t>(camera) + Game::CameraPositionOffset);
                candidate = D3DXVECTOR3(pos[0], pos[1], pos[2]);
                hasCandidate = (candidate != D3DXVECTOR3(0, 0, 0));
            }

            if (hasCandidate)
            {
                m_cameraPtr = camera;
                if (active)
                    return candidate;
                fallbackPos = candidate;
                hasFallback = true;
            }
        }

        if (hasFallback)
            return fallbackPos;
    }
    else if (detected_game == GameType::UC)
    {
        auto head = reinterpret_cast<EViewNode*>(Game::EViewListHeadPtr);
        if (head)
        {
            auto mat = reinterpret_cast<D3DXMATRIX*>(
                reinterpret_cast<uintptr_t>(head) + Game::NodeMatrixOffset);
            return D3DXVECTOR3(mat->_41, mat->_42, mat->_43);
        }
    }
    else if (detected_game == GameType::PS)
    {
        if (g_EVIEW_LIST_PTR == nullptr)
        {
            DebugEVIEWListPtr();
        }

        if (g_EVIEW_LIST_PTR)
        {
            auto head = *g_EVIEW_LIST_PTR;
            if (head)
            {
                auto mat = reinterpret_cast<D3DXMATRIX*>(
                    reinterpret_cast<uintptr_t>(head) + Game::NodeMatrixOffset);
                return D3DXVECTOR3(mat->_41, mat->_42, mat->_43);
            }
        }
    }

    if (m_device)
    {
        D3DXMATRIX view;
        if (SUCCEEDED(m_device->GetTransform(D3DTS_VIEW, &view)))
        {
            D3DXMATRIX invView;
            if (D3DXMatrixInverse(&invView, nullptr, &view))
                return D3DXVECTOR3(invView._41, invView._42, invView._43);
        }
    }

    RainDebugOut("[GetCameraPositionSafe] 🚫 No valid camera source yet.\n");
    return D3DXVECTOR3(0, 0, 0);
}

bool PrecipitationController::IsCameraCovered(const D3DXVECTOR3& camPos)
{
    // You can add more zones or load from config later
    if ((camPos.x > g_precipitationConfig.occlusionZone_XMin && camPos.x < g_precipitationConfig.occlusionZone_XMax) &&
        (camPos.z > g_precipitationConfig.occlusionZone_ZMin && camPos.z < g_precipitationConfig.occlusionZone_ZMax))
    {
        return true;
    }

    return false;
}


static bool GetViewProjFromCameraParams(void* cameraPtr, const D3DVIEWPORT9& vp, D3DXMATRIX& outView, D3DXMATRIX& outProj)
{
    if (!cameraPtr || !IsLikelyPointer(cameraPtr))
        return false;

    if (!GetViewFromCameraParams(cameraPtr, outView))
        return false;
    if (!GetProjFromCameraParams(cameraPtr, vp, outProj))
        return false;
    return true;
}

static bool GetRainVolumeBounds(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax)
{
    auto* rain = *reinterpret_cast<void**>(Game::RainInstancePtr);
    if (!rain)
        return false;
    auto minPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rain) + 0x3830);
    auto maxPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rain) + 0x3840);
    if (!core::IsReadable(minPtr, sizeof(D3DXVECTOR3)) || !core::IsReadable(maxPtr, sizeof(D3DXVECTOR3)))
        return false;
    std::memcpy(&outMin, minPtr, sizeof(D3DXVECTOR3));
    std::memcpy(&outMax, maxPtr, sizeof(D3DXVECTOR3));
    return true;
}

static bool GetCameraPosFromCamParams(void* camParams, D3DXVECTOR3& outPos)
{
    if (!camParams)
        return false;
    auto* camMat = reinterpret_cast<D3DXMATRIX*>(reinterpret_cast<uintptr_t>(camParams) + 0x50);
    if (!core::IsReadable(camMat, sizeof(D3DXMATRIX)))
        return false;
    D3DXMATRIX m = *camMat;
    outPos = D3DXVECTOR3(m._41, m._42, m._43);
    return true;
}

void PrecipitationController::enable()
{
    if (m_active)
        return;

    RainDebugOut("[PrecipitationController::enable] enabling\n");
    RainConfigController::Load();
    m_active = true;

    if (!m_device && Game::NFS_D3D9_DEVICE_ADDRESS)
    {
        auto** devPtr = reinterpret_cast<IDirect3DDevice9**>(Game::NFS_D3D9_DEVICE_ADDRESS);
        if (core::IsReadable(devPtr, sizeof(void*)) && *devPtr)
        {
            m_device = *devPtr;
            OutputDebugStringA("[PrecipitationController::enable] m_device resolved from NFS_D3D9_DEVICE_ADDRESS\n");
        }
    }

    if (g_precipitationConfig.fpsOverride > 0.0f)
        core::fpsDeltaTime = 1.0f / g_precipitationConfig.fpsOverride;
    else
        m_lastTime = timeGetTime(); // where m_lastTime is a DWORD member

    // Select rain texture format
    chosenFormat = D3DFMT_A8R8G8B8;
    if (detected_game == GameType::UC)
    {
        // chosenFormat = GetSupportedRainTextureFormat(m_device);
        // if (chosenFormat == D3DFMT_UNKNOWN)
        // {
        //     RainDebugOut("[RainTex] No supported texture format found\n");
        //     return false;
        // }
        // chosenFormat = D3DFMT_A8B8G8R8;
    }

    // Resize drop containers if first-time

    if (m_drops2D.empty()) m_drops2D.resize(g_precipitationConfig.drop2DCount);
    // m_drops2D.clear(); // important

    int drops3dSize = g_precipitationConfig.dropCountNear + g_precipitationConfig.dropCountMid + g_precipitationConfig.
        dropCountFar;
    m_drops3D.resize(drops3dSize);
    m_splatters3D.resize(drops3dSize / 3);
    for (auto& s : m_splatters3D)
        s.alive = false;
    m_splatterWrite = 0;

    // Get camera position early for consistent reference
    static bool printedCameraReady = false;
    camPos = GetCameraPositionSafe();
    if (camPos != D3DXVECTOR3(0, 0, 0))
    {
        if (!printedCameraReady)
        {
            RainDebugOut("[PrecipitationController::enable] 🎥 Camera is now valid.\n");
            printedCameraReady = true;
        }
        m_cameraY = GetUpCoord(camPos);
    }

    // camPos = GetCameraPositionSafe();
    // m_cameraY = camPos.y;

    // Prepare rain group settings based on intensity
    ScaleSettingsForIntensity(g_precipitationConfig.rainIntensity);

    // Local lambda to spawn a splatter at a given drop position
    auto spawnSplatter = [&](const D3DXVECTOR3& pos)
    {
    Drop3D splatter;
    splatter.position = pos;
    SetUpCoord(splatter.position, m_cameraY - 1.0f);

        // splatter.velocity = D3DXVECTOR3(0, 0, 0);
        splatter.velocity = D3DXVECTOR3(0, 0, 0);
        SetUpCoord(splatter.velocity, -3.0f); // or adjustable by config

        splatter.length = 4.0f + static_cast<float>(RandRangeInt(0, 2));
        splatter.life = 3.5f + RandFloat01();
        splatter.angle = static_cast<float>(RandRangeInt(0, 359)) * (D3DX_PI / 180.0f);
        return splatter;
    };

    // Preallocate the drops with separate logic per group
    // Generate initial 3D splatters (fewer than raindrops)
    size_t dropIndex = 0;
    for (int group = 0; group < 3; ++group)
    {
        const RainGroupSettings& settings = m_rainSettings[group];
        for (int i = 0; i < settings.dropCount && dropIndex < m_drops3D.size(); ++i, ++dropIndex)
        {
            Drop3D drop = RespawnDrop(settings, m_cameraY);
            drop.alive = true;
            m_drops3D[dropIndex] = drop;
        }

        for (int i = 0; i < settings.dropCount / 2; ++i)
        {
            Drop3D splat = spawnSplatter(RespawnDrop(settings, m_cameraY).position);
            splat.alive = true;
            if (!m_splatters3D.empty())
            {
                m_splatters3D[m_splatterWrite] = splat;
                m_splatterWrite = (m_splatterWrite + 1) % m_splatters3D.size();
            }
        }
    }

    // Do not register a post-Present DX9 loop; we render in HookedPresent only.
}

void PrecipitationController::disable()
{
    RainDebugOut("[PrecipitationController::disable] disabling\n");
    g_precipitationConfig = RainConfigController::PrecipitationData();
    // resets all members to their default values

    m_active = false;
    m_drops2D.clear();
    m_drops3D.clear();
    m_splatters3D.clear();

    // No DX9 loop registered anymore.
}

PrecipitationController::Drop3D PrecipitationController::RespawnDrop(const RainGroupSettings& settings, float camY)
{
    Drop3D drop;
    drop.length = settings.dropSize;
    drop.velocity = D3DXVECTOR3(settings.windSway, 0.0f, 0.0f);
    SetUpCoord(drop.velocity, -settings.speed);
    if (detected_game == GameType::MW || detected_game == GameType::CB)
    {
        D3DXVECTOR3 mn{}, mx{};
        if (GetRainVolumeBounds(mn, mx))
        {
            float rx = RandFloat01();
            float ry = RandFloat01();
            float rz = RandFloat01();
            drop.position = D3DXVECTOR3(
                mn.x + (mx.x - mn.x) * rx,
                mn.y + (mx.y - mn.y) * ry,
                mn.z + (mx.z - mn.z) * rz);
        }
        else
        {
            drop.position = D3DXVECTOR3(
                camPos.x + static_cast<float>(RandRangeInt(-200, 199)),
                0.0f,
                camPos.z + static_cast<float>(RandRangeInt(-200, 799)));
            SetUpCoord(drop.position, camY + 25.0f);
        }
    }
    else
    {
        drop.position = D3DXVECTOR3(
            camPos.x + static_cast<float>(RandRangeInt(-200, 199)),
            0.0f,
            camPos.z + static_cast<float>(RandRangeInt(-200, 799)));
        SetUpCoord(drop.position, camY + 25.0f);
    }
    drop.life = 3.5f + RandFloat01();
    drop.angle = static_cast<float>(RandRangeInt(0, 359)) * (D3DX_PI / 180.0f);
    return drop;
}

const PrecipitationController::RainGroupSettings* PrecipitationController::ChooseGroupByY(float y)
{
    float relY = y - m_cameraY;
    if (relY > 85.0f)
        return &m_rainSettings[0]; // near
    if (relY > 55.0f)
        return &m_rainSettings[1]; // mid

    return &m_rainSettings[2]; // far
}

void PrecipitationController::ScaleSettingsForIntensity(float intensity)
{
    float intensityCurve = powf(intensity, 1.2f);
    m_rainSettings[0].dropCount = static_cast<int>(g_precipitationConfig.dropCountNear * intensity);
    m_rainSettings[1].dropCount = static_cast<int>(g_precipitationConfig.dropCountMid * intensity);
    m_rainSettings[2].dropCount = static_cast<int>(g_precipitationConfig.dropCountFar * intensity);

    m_rainSettings[0].dropSize = g_precipitationConfig.dropSizeNear;
    m_rainSettings[1].dropSize = g_precipitationConfig.dropSizeMid;
    m_rainSettings[2].dropSize = g_precipitationConfig.dropSizeFar;

    m_rainSettings[0].speed = g_precipitationConfig.speedNear + intensityCurve;
    m_rainSettings[1].speed = g_precipitationConfig.speedMid + intensityCurve;
    m_rainSettings[2].speed = g_precipitationConfig.speedFar + intensityCurve;

    m_rainSettings[0].windSway = g_precipitationConfig.windSwayNear;
    m_rainSettings[1].windSway = g_precipitationConfig.windSwayMid;
    m_rainSettings[2].windSway = g_precipitationConfig.windSwayFar;

    m_rainSettings[0].alphaBlended = g_precipitationConfig.alphaBlend3DRainNear;
    m_rainSettings[1].alphaBlended = g_precipitationConfig.alphaBlend3DRainMid;
    m_rainSettings[2].alphaBlended = g_precipitationConfig.alphaBlend3DRainFar;
}

bool PrecipitationController::IsCreatedRainTexture()
{
    if (!m_device)
        return false;

    if (m_rainTex)
    {
        m_rainTex->Release();
        m_rainTex = nullptr;
    }

    HRESULT hr = m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_MANAGED, &m_rainTex, nullptr);
    if (FAILED(hr))
    {
        hr = m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_DEFAULT, &m_rainTex, nullptr);
    }

    if (FAILED(hr) || !m_rainTex)
    {
        char buf[128];
        sprintf_s(buf, "[IsCreatedRainTexture] CreateTexture failed hr=0x%08X\n", static_cast<unsigned>(hr));
        RainDebugOut(buf);
        return false;
    }
    // m_device->CreateTexture(16, 512, 1, 0, chosenFormat, D3DPOOL_MANAGED, &m_rainTex, nullptr);

    D3DLOCKED_RECT rect;
    bool result = SUCCEEDED(m_rainTex->LockRect(0, &rect, nullptr, 0));
    if (result)
    {
        // First pass: thin rain lines (near group with fewer & smaller streaks)
        for (int x = 6; x < 26; x += 16)
        {
            int startY = RandRangeInt(0, 256 - 13);
            for (int y = 0; y < 12; ++y)
            {
                DWORD* row = reinterpret_cast<DWORD*>((BYTE*)rect.pBits + (startY + y) * rect.Pitch);
                BYTE alpha = static_cast<BYTE>(min(255, 192 - y * 6));
                row[x] = (alpha << 24) | 0x80FFFFFF; // semi-transparent white
            }
        }

        // Second pass: thick core streaks (mid group)
        for (int x = 10; x < 22; x += 10)
        {
            int startY = RandRangeInt(0, 256 - 33);
            for (int y = 0; y < 32; ++y)
            {
                DWORD* row = reinterpret_cast<DWORD*>((BYTE*)rect.pBits + (startY + y) * rect.Pitch);
                float normY = static_cast<float>(startY + y) / 256.0f;
                float fade = 1.0f - std::clamp(normY / (2.0f / 3.0f), 0.0f, 1.0f);
                BYTE alpha = static_cast<BYTE>(min(255.0f, (224 - y * 3) * fade * 2.0f));

                if (x > 0) row[x - 1] = (alpha / 2 << 24) | 0xFFFFFF;
                row[x] = (alpha << 24) | 0xFFFFFF;
                if (x + 1 < 32) row[x + 1] = (alpha / 2 << 24) | 0xFFFFFF;
            }
        }

        // Third pass: high-alpha streaks (far group, smaller & faint)
        for (int x = 0; x < 32; x += 6)
        {
            int startY = RandRangeInt(0, 256 - 9);
            for (int y = 0; y < 8; ++y)
            {
                DWORD* row = reinterpret_cast<DWORD*>((BYTE*)rect.pBits + (startY + y) * rect.Pitch);
                float normY = static_cast<float>(startY + y) / 256.0f;
                float fade = 1.0f - std::clamp(normY / 0.5f, 0.0f, 1.0f);
                BYTE alpha = static_cast<BYTE>((160 - y * 2) * fade);

                row[x] = (alpha << 24) | 0xFFFFFF;
                if (x > 0) row[x - 1] = ((alpha / 2) << 24) | 0xFFFFFF;
                if (x + 1 < 32) row[x + 1] = ((alpha / 2) << 24) | 0xFFFFFF;
            }
        }

        m_rainTex->UnlockRect(0);
        RainDebugOut("[IsCreatedRainTexture] Vertical rain streaks texture created\n");
    }
    else
        RainDebugOut("[IsCreatedRainTexture] Failed Procedural fallback rain texture creation\n");

    return result;
}

void PrecipitationController::Render3DRainOverlay(const D3DVIEWPORT9& viewport)
{
    static bool loggedRender3D = false;
    if (!loggedRender3D)
    {
        OutputDebugStringA("[PrecipitationController::Render3DRainOverlay] entered\n");
        loggedRender3D = true;
    }
    static bool renderEntryLogged = false;
    if (!renderEntryLogged)
    {
        static bool loggedRender3DEntry = false;
        if (!loggedRender3DEntry)
        {
            RainDebugOut("[RainDebug] Render3D entry\n");
            loggedRender3DEntry = true;
        }
        renderEntryLogged = true;
    }
    if (!m_rainTex)
    {
        if (g_precipitationConfig.use_raindrop_dds)
        {
            if (FAILED(
                D3DXCreateTextureFromFileA(m_device, g_precipitationConfig.raindropTexturePath.c_str(), &m_rainTex)))
            {
                RainDebugOut("[Render3DRainOverlay] Failed to load raindrop.dds\n");
                IsCreatedRainTexture();
            }
        }
        else
        {
            IsCreatedRainTexture();
        }

        if (!m_rainTex)
            return;
    }

    D3DXMATRIX matView, matProj, matIdentity;
    D3DXMATRIX savedView{}, savedProj{}, savedWorld{};
    bool restoreTransforms = false;
    bool hasProj = false;
    bool useViewProjOnly = false;
    D3DXMATRIX matViewProj{};
    D3DXMATRIX matViewProjT{};
    bool loggedVP = false;
    bool transposeVP = false;
    void* camPtr = nullptr;
    bool locked = false;
    
    // Estimated camera position (replace this with actual camera lookup if possible)
    // D3DXVECTOR3 camPos(0, 0, 0); // TODO: Replace with real camera pos if found
    D3DXVECTOR3 camPos = GetCameraPositionSafe();
    static bool loggedFirstDropPos = false;

    
    GetFrameViewProj(viewport, matView, matProj, hasProj, locked);
    D3DXMatrixIdentity(&matIdentity);

    if (useViewProjOnly)
    {
        D3DXMatrixMultiply(&matViewProj, &matView, &matProj);
        D3DXMatrixTranspose(&matViewProjT, &matViewProj);
    }

    if (detected_game == GameType::MW)
    {
        // Force device transforms to match MW view/proj path for world-space rain.
        if (SUCCEEDED(m_device->GetTransform(D3DTS_VIEW, &savedView)) &&
            SUCCEEDED(m_device->GetTransform(D3DTS_PROJECTION, &savedProj)) &&
            SUCCEEDED(m_device->GetTransform(D3DTS_WORLD, &savedWorld)))
        {
            restoreTransforms = true;
        }
        m_device->SetTransform(D3DTS_WORLD, &matIdentity);
        m_device->SetTransform(D3DTS_VIEW, &matView);
        if (hasProj)
            m_device->SetTransform(D3DTS_PROJECTION, &matProj);
    }


    // ✅ Ensure correct alpha blending and texture state before drawing
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, detected_game == GameType::MW ? FALSE : TRUE);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_ALPHAREF, 8);
    bool useWorldSpace = (detected_game == GameType::MW);
    if (useWorldSpace)
        m_device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    else
        m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    if (detected_game == GameType::UC)
    {
        m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    }

    m_device->SetTexture(0, m_rainTex);

    const auto ProjectFast = [&](const D3DXVECTOR3& world, const D3DVIEWPORT9& vp,
                                 const D3DXMATRIX& vpMat, D3DXVECTOR3& outScreen) -> bool
    {
        D3DXVECTOR4 v(world.x, world.y, world.z, 1.0f);
        D3DXVECTOR4 clip{};
        D3DXVec4Transform(&clip, &v, &vpMat);
        if (clip.w == 0.0f)
            return false;
        const float invW = 1.0f / clip.w;
        const float ndcX = clip.x * invW;
        const float ndcY = clip.y * invW;
        const float ndcZ = clip.z * invW;
        outScreen.x = vp.X + (ndcX + 1.0f) * 0.5f * vp.Width;
        outScreen.y = vp.Y + (1.0f - ndcY) * 0.5f * vp.Height;
        outScreen.z = (ndcZ + 1.0f) * 0.5f;
        return true;
    };

    // Move drops
    D3DXVECTOR3 windDir(1.0f, 0.0f, 0.0f);
    if (detected_game == GameType::MW || detected_game == GameType::CB)
    {
        D3DXVECTOR3 camRight{}, camUp{}, camForward{};
        void* camParams = GetCameraParamsFromEView();
        if (camParams && GetCameraBasisFromCamParams(camParams, camRight, camUp, camForward))
            windDir = camForward;
        else if (camPtr && GetCameraBasisFromStruct(camPtr, camRight, camUp, camForward))
            windDir = camForward;
        else
            windDir = D3DXVECTOR3(-matView._31, -matView._32, -matView._33);
        float len2 = windDir.x * windDir.x + windDir.y * windDir.y + windDir.z * windDir.z;
        if (len2 > 1e-6f)
            D3DXVec3Normalize(&windDir, &windDir);
        else
            windDir = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
    }
    D3DXVECTOR3 forwardXZ(0.0f, 0.0f, 1.0f);
    {
        D3DXMATRIX invView{};
        if (D3DXMatrixInverse(&invView, nullptr, &matView))
        {
            D3DXVECTOR3 camForward(invView._31, invView._32, invView._33);
            forwardXZ = D3DXVECTOR3(camForward.x, 0.0f, camForward.z);
            if (D3DXVec3LengthSq(&forwardXZ) < 0.0001f)
                forwardXZ = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
            else
                D3DXVec3Normalize(&forwardXZ, &forwardXZ);
        }
    }

    const int windFrameId = static_cast<int>(core::CurrentTime / 66);
    for (auto& drop : m_drops3D)
    {
        const RainGroupSettings* group = ChooseGroupByY(GetUpCoord(drop.position));

        float up = GetUpCoord(drop.position);
        if (drop.windFrame != windFrameId)
        {
            drop.windCache = (noise.noise3D(up * 0.05f, core::CurrentTime * 0.0005f, 0.0f) - 0.5f) * 2.0f;
            drop.windFrame = windFrameId;
        }
        float wind = drop.windCache * group->windSway;
        if (detected_game == GameType::MW || detected_game == GameType::CB)
            drop.position += windDir * wind;
        else
            drop.position.x += wind;
        SetUpCoord(drop.position, up - group->speed);

        if (GetUpCoord(drop.position) < m_cameraY - 10.0f)
        {
            Drop3D splatter;

            float randX = static_cast<float>(RandRangeInt(-50, 49)) * 0.05f;
            float randZ = static_cast<float>(RandRangeInt(-50, 49)) * 0.5f;
            D3DXVECTOR3 offset = forwardXZ * 6.0f + D3DXVECTOR3(randX, 0.0f, randZ);

            D3DXVECTOR3 splatterPos = camPos + offset;
            SetUpCoord(splatterPos, m_cameraY - 1.0f);

            splatter.position = splatterPos;
            splatter.velocity = D3DXVECTOR3(0, -3.0f, 0);
            splatter.length = 4.0f + static_cast<float>(RandRangeInt(0, 2));
            splatter.life = 3.5f + RandFloat01();
            splatter.angle = atan2f(forwardXZ.x, forwardXZ.z);

            // char dbg[128];
            // sprintf_s(dbg, "[Splatter] forwardXZ: %.2f %.2f %.2f\n", forwardXZ.x, forwardXZ.y, forwardXZ.z);
            // RainDebugOut(dbg);

            splatter.alive = true;
            if (!m_splatters3D.empty())
            {
                m_splatters3D[m_splatterWrite] = splatter;
                m_splatterWrite = (m_splatterWrite + 1) % m_splatters3D.size();
            }
            drop = RespawnDrop(*group, m_cameraY);
        }
    }

    auto RenderGroup = [&](float minY, float maxY, bool enableBlend, int alpha)
    {
        m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, enableBlend);

        D3DXMATRIX viewUse = matView;
        D3DXMATRIX projUse = matProj;
        D3DXVECTOR3 right(1.0f, 0.0f, 0.0f);
        D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
        if (useWorldSpace)
        {
            // Use the already-selected view/proj (camera params preferred for MW)
            m_device->SetTransform(D3DTS_VIEW, &viewUse);
            m_device->SetTransform(D3DTS_PROJECTION, &projUse);
            m_device->SetTransform(D3DTS_WORLD, &matIdentity);
            D3DXMATRIX invViewUse{};
            if (D3DXMatrixInverse(&invViewUse, nullptr, &viewUse) != nullptr)
            {
                right = D3DXVECTOR3(invViewUse._11, invViewUse._12, invViewUse._13);
                up = D3DXVECTOR3(invViewUse._21, invViewUse._22, invViewUse._23);
                if (D3DXVec3LengthSq(&right) > 1e-6f)
                    D3DXVec3Normalize(&right, &right);
                if (D3DXVec3LengthSq(&up) > 1e-6f)
                    D3DXVec3Normalize(&up, &up);
            }
            m_device->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        }
        else
        {
            m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
        }

        std::vector<RHWVertex> rhwVerts;
        std::vector<WorldVertex> worldVerts;
        if (useWorldSpace)
            worldVerts.reserve(m_drops3D.size() * 6);
        else
            rhwVerts.reserve(m_drops3D.size() * 6);

        for (const auto& drop : m_drops3D)
        {
            if (GetUpCoord(drop.position) < minY || GetUpCoord(drop.position) >= maxY)
                continue;

        if (!loggedFirstDropPos)
        {
            char buf[256];
            sprintf_s(buf, "[RainDebug] dropPos=(%.2f,%.2f,%.2f)\n",
                      drop.position.x, drop.position.y, drop.position.z);
            RainDebugOut(buf);
            loggedFirstDropPos = true;
        }
        float size = drop.length * 4.0f;
        float half = size * 0.5f;
        DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

        float cosA = cosf(drop.angle);
        float sinA = sinf(drop.angle);

        if (!useWorldSpace)
        {
            D3DXVECTOR3 screen;
            if (useViewProjOnly)
            {
                D3DXVECTOR3 screenAlt{};
                bool okA = ProjectFast(drop.position, viewport, transposeVP ? matViewProjT : matViewProj, screen);
                bool okB = ProjectFast(drop.position, viewport, transposeVP ? matViewProj : matViewProjT, screenAlt);
                if (!okA && okB)
                {
                    screen = screenAlt;
                    okA = true;
                }
                if (!okA)
                    continue;
                if (!loggedVP)
                {
                    DebugLogProjectOnce("Render3D VP", viewport, matViewProj, drop.position, screen);
                    if (okB)
                    {
                        char buf[256];
                        sprintf_s(buf, "[RainDebug] Render3D VP alt screen=(%.2f,%.2f,%.2f)\n",
                                  screenAlt.x, screenAlt.y, screenAlt.z);
                        RainDebugOut(buf);
                    }
                    loggedVP = true;
                }
            }
            else
            {
                D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matIdentity);
            }

            if (screen.z < 0.0f || screen.z > 1.0f)
                continue;

            D3DXVECTOR2 center(screen.x, screen.y);
            D3DXVECTOR2 offsets[4] = {{-half, -half}, {half, -half}, {half, half}, {-half, half}};
            RHWVertex quad[4];

            for (int i = 0; i < 4; ++i)
            {
                float x = offsets[i].x * cosA - offsets[i].y * sinA;
                float y = offsets[i].x * sinA + offsets[i].y * cosA;
                quad[i] = {
                    center.x + x, center.y + y, screen.z, 1.0f, color, (i == 1 || i == 2) ? 1.0f : 0.0f,
                    (i >= 2) ? 1.0f : 0.0f
                };
            }

            rhwVerts.push_back(quad[0]);
            rhwVerts.push_back(quad[1]);
            rhwVerts.push_back(quad[2]);
            rhwVerts.push_back(quad[0]);
            rhwVerts.push_back(quad[2]);
            rhwVerts.push_back(quad[3]);
        }
        else
        {
            D3DXVECTOR3 r = right * (half * cosA) + up * (half * sinA);
            D3DXVECTOR3 u = right * (-half * sinA) + up * (half * cosA);

            D3DXVECTOR3 worldPos = drop.position;
            WorldVertex quad[4] = {
                {worldPos.x - r.x - u.x, worldPos.y - r.y - u.y, worldPos.z - r.z - u.z, color, 0.0f, 0.0f},
                {worldPos.x + r.x - u.x, worldPos.y + r.y - u.y, worldPos.z + r.z - u.z, color, 1.0f, 0.0f},
                {worldPos.x + r.x + u.x, worldPos.y + r.y + u.y, worldPos.z + r.z + u.z, color, 1.0f, 1.0f},
                {worldPos.x - r.x + u.x, worldPos.y - r.y + u.y, worldPos.z - r.z + u.z, color, 0.0f, 1.0f},
            };
            worldVerts.push_back(quad[0]);
            worldVerts.push_back(quad[1]);
            worldVerts.push_back(quad[2]);
            worldVerts.push_back(quad[0]);
            worldVerts.push_back(quad[2]);
            worldVerts.push_back(quad[3]);
        }
        }

        if (useWorldSpace)
            DrawDynamicVB(m_device, g_worldQuadVB, worldVerts, D3DPT_TRIANGLELIST,
                          static_cast<UINT>(worldVerts.size() / 3));
        else
            DrawDynamicVB(m_device, g_rhwQuadVB, rhwVerts, D3DPT_TRIANGLELIST,
                          static_cast<UINT>(rhwVerts.size() / 3));
    };

    const float nearMin = m_cameraY + g_precipitationConfig.nearMinOffset;
    const float nearMax = m_cameraY + g_precipitationConfig.nearMaxOffset;
    const float midMax = m_cameraY + g_precipitationConfig.midMaxOffset;
    const float farMax = m_cameraY + g_precipitationConfig.farMaxOffset;

    RenderGroup(nearMin, nearMax, g_precipitationConfig.alphaBlend3DRainNear,
                g_precipitationConfig.alphaBlendNearValue);
    RenderGroup(nearMax, midMax, g_precipitationConfig.alphaBlend3DRainMid,
                g_precipitationConfig.alphaBlendMidValue);
    RenderGroup(midMax, farMax, g_precipitationConfig.alphaBlend3DRainFar,
                g_precipitationConfig.alphaBlendFarValue);

    // Fixed-size pools avoid erase churn.

    m_device->SetTexture(0, nullptr);

    if (restoreTransforms)
    {
        m_device->SetTransform(D3DTS_WORLD, &savedWorld);
        m_device->SetTransform(D3DTS_VIEW, &savedView);
        m_device->SetTransform(D3DTS_PROJECTION, &savedProj);
    }
}

void PrecipitationController::Render3DSplattersOverlay(const D3DVIEWPORT9& viewport)
{
    static bool splatEntryLogged = false;
    if (!splatEntryLogged)
    {
        static bool loggedSplattersEntry = false;
        if (!loggedSplattersEntry)
        {
            RainDebugOut("[RainDebug] Splatters entry\n");
            loggedSplattersEntry = true;
        }
        splatEntryLogged = true;
    }
    void* camPtr = PrecipitationController::Get()->m_cameraPtr;
    if (!camPtr)
        camPtr = GetCameraPtrFromEView();
    // ✅ Ensure correct alpha blending and texture state before drawing
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_ALPHAREF, 8);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    if (detected_game == GameType::UC)
    {
        m_device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    }

    // Create splatter texture if not available
    if (!m_splatterTex)
    {
        m_device->CreateTexture(16, 16, 1, 0, chosenFormat, D3DPOOL_DEFAULT, //if issues happen, try D3DPOOL_MANAGED
                                &m_splatterTex, nullptr);
        // m_device->CreateTexture(4, 4, 1, 0, chosenFormat, D3DPOOL_DEFAULT, &m_splatterTex, nullptr);
        D3DLOCKED_RECT rect;
        if (SUCCEEDED(m_splatterTex->LockRect(0, &rect, nullptr, 0)))
        {
            DWORD* pixels = static_cast<DWORD*>(rect.pBits);
            for (int i = 0; i < 16; ++i)
                pixels[i] = 0x80FFFFFF; // semi-transparent white
            m_splatterTex->UnlockRect(0);
            RainDebugOut("[Rain] Procedural fallback splatter texture created\n");
        }
    }

    // Setup matrices
    D3DXMATRIX matProj, matView, matWorld;
    D3DXMATRIX savedView{}, savedProj{}, savedWorld{};
    bool restoreTransforms = false;
    D3DXMatrixIdentity(&matWorld);
    bool hasProj = false;
    bool useViewProjOnly = false;
    D3DXMATRIX matViewProj{};
    bool loggedVP = false;
    bool transposeVP = false;
    bool locked = false;
    GetFrameViewProj(viewport, matView, matProj, hasProj, locked);

    D3DXMatrixIdentity(&matWorld);

    if (detected_game == GameType::MW)
    {
        if (SUCCEEDED(m_device->GetTransform(D3DTS_VIEW, &savedView)) &&
            SUCCEEDED(m_device->GetTransform(D3DTS_PROJECTION, &savedProj)) &&
            SUCCEEDED(m_device->GetTransform(D3DTS_WORLD, &savedWorld)))
        {
            restoreTransforms = true;
        }
        m_device->SetTransform(D3DTS_WORLD, &matWorld);
        m_device->SetTransform(D3DTS_VIEW, &matView);
        if (hasProj)
            m_device->SetTransform(D3DTS_PROJECTION, &matProj);
    }
    static bool loggedFinal = false;
    if (!loggedFinal)
    {
        DebugLogMatrixNoGuard("Render3D final view", matView);
        if (hasProj)
            DebugLogMatrixNoGuard("Render3D final proj", matProj);
        char buf[128];
        sprintf_s(buf, "[RainDebug] Render3D hasProj=%d drops=%zu\n", hasProj ? 1 : 0, m_drops3D.size());
        RainDebugOut(buf);
        loggedFinal = true;
    }

    // Set render state
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, g_precipitationConfig.alphaBlendSplatters);
    m_device->SetTexture(0, m_splatterTex);

    // Update splatter positions and life (swap-remove to avoid memmove)
    for (size_t i = 0; i < m_splatters3D.size();)
    {
        auto& splat = m_splatters3D[i];
        if (!splat.alive)
        {
            ++i;
            continue;
        }
        splat.position += splat.velocity * core::fpsDeltaTime;
        splat.life -= core::fpsDeltaTime;

        if (splat.life <= 0.0f)
        {
            splat.alive = false;
            ++i;
        }
        else
        {
            ++i;
        }
    }

    bool anyAlive = false;
    for (const auto& splat : m_splatters3D)
    {
        if (splat.alive)
        {
            anyAlive = true;
            break;
        }
    }
    if (!anyAlive)
        return;

    std::vector<RHWVertex> rhwVerts;
    rhwVerts.reserve(m_splatters3D.size() * 6);

    for (const auto& drop : m_splatters3D)
    {
        if (!drop.alive)
            continue;
        D3DXVECTOR3 screen{};
        if (useViewProjOnly)
        {
            if (!ProjectWithViewProj(drop.position, viewport, matViewProj, screen, transposeVP))
                continue;
            if (!loggedVP)
            {
                DebugLogProjectOnce("Splatters VP", viewport, matViewProj, drop.position, screen);
                loggedVP = true;
            }
        }
        else
        {
            D3DXVec3Project(&screen, &drop.position, &viewport, &matProj, &matView, &matWorld);
        }

        if (screen.z < 0.0f || screen.z > 1.0f)
            continue;

        float flicker = static_cast<float>(RandRangeInt(0, 99)) / 500.0f; // up to ±0.2
        float size = std::clamp((drop.length + flicker) * 4.0f, 1.0f, 6.5f);
        float half = size * 0.5f;

        float cosA = cosf(drop.angle);
        float sinA = sinf(drop.angle);

        D3DXVECTOR2 center(screen.x, screen.y);
        D3DXVECTOR2 offsets[4] = {
            {-half, -half},
            {half, -half},
            {half, half},
            {-half, half}
        };

        // Fade alpha by life (optional: multiply with intensity later)
        BYTE alpha = static_cast<BYTE>(std::clamp(drop.life / 2.0f, 0.0f, 1.0f) * 255.0f);
        DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

        RHWVertex quad[4];
        for (int i = 0; i < 4; ++i)
        {
            float x = offsets[i].x * cosA - offsets[i].y * sinA;
            float y = offsets[i].x * sinA + offsets[i].y * cosA;

            quad[i] = {
                center.x + x,
                center.y + y,
                screen.z,
                1.0f,
                color,
                (i == 1 || i == 2) ? 1.0f : 0.0f,
                (i >= 2) ? 1.0f : 0.0f
            };
        }

        rhwVerts.push_back(quad[0]);
        rhwVerts.push_back(quad[1]);
        rhwVerts.push_back(quad[2]);
        rhwVerts.push_back(quad[0]);
        rhwVerts.push_back(quad[2]);
        rhwVerts.push_back(quad[3]);
    }

    DrawDynamicVB(m_device, g_rhwQuadVB, rhwVerts, D3DPT_TRIANGLELIST,
                  static_cast<UINT>(rhwVerts.size() / 3));

    m_device->SetTexture(0, nullptr);

    if (restoreTransforms)
    {
        m_device->SetTransform(D3DTS_WORLD, &savedWorld);
        m_device->SetTransform(D3DTS_VIEW, &savedView);
        m_device->SetTransform(D3DTS_PROJECTION, &savedProj);
    }
}

void PrecipitationController::Render2DRainOverlay(const D3DVIEWPORT9& viewport)
{
    if (g_precipitationConfig.rainIntensity == 0.0f)
        return;

    float width = static_cast<float>(viewport.Width);
    float height = static_cast<float>(viewport.Height);

    BYTE alpha = static_cast<BYTE>(std::clamp(
        g_precipitationConfig.alpha2DRainMin + g_precipitationConfig.rainIntensity *
        (g_precipitationConfig.alpha2DRainMax - g_precipitationConfig.alpha2DRainMin), 0.0f, 255.0f));

    DWORD color = D3DCOLOR_ARGB(alpha, 255, 255, 255);

    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, g_precipitationConfig.alphaBlend2DRain);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    m_device->SetTexture(0, nullptr);

    std::vector<LineVertex> lineVerts;
    lineVerts.reserve(m_drops2D.size() * 2);

    for (auto& drop : m_drops2D)
    {
        if (!drop.initialized)
        {
            drop.x = RandFloat01() * width;
            drop.y = RandFloat01() * height;
            drop.speed = g_precipitationConfig.baseSpeed + g_precipitationConfig.rainIntensity * g_precipitationConfig.
                speedScale;
            drop.length = g_precipitationConfig.baseLength + g_precipitationConfig.rainIntensity * g_precipitationConfig
                .lengthScale;
            drop.noiseSeed = RandFloat01() * 100.0f;
            drop.initialized = true;
        }

        drop.y += drop.speed * 0.016f;
        float wind = (PrecipitationController::noise.noise3D(drop.y * 0.05f, core::CurrentTime * 0.0005f,
                                                             drop.noiseSeed) -
                0.5f) * 2.0f * g_precipitationConfig.
            windStrength;
        drop.x += wind * 0.016f;

        if (drop.y > height)
        {
            drop.x = RandFloat01() * width;
            drop.y = -drop.length;
        }

        LineVertex verts[2] = {
            {drop.x, drop.y, 0.0f, 1.0f, color},
            {drop.x, drop.y + drop.length, 0.0f, 1.0f, color}
        };

        lineVerts.push_back(verts[0]);
        lineVerts.push_back(verts[1]);
    }

    DrawDynamicVB(m_device, g_lineVB, lineVerts, D3DPT_LINELIST,
                  static_cast<UINT>(lineVerts.size() / 2));
}

void PrecipitationController::Update()
{
    static bool loggedUpdate = false;
    if (!loggedUpdate)
    {
        char buf[256];
        sprintf_s(buf,
            "[PrecipitationController::Update] active=%d device=%p applyPresetRendering=%d\n",
            m_active ? 1 : 0,
            m_device,
            g_precipitationConfig.applyPresetRendering ? 1 : 0);
        OutputDebugStringA(buf);
        loggedUpdate = true;
    }

    if (!m_device && Game::NFS_D3D9_DEVICE_ADDRESS)
    {
        auto** devPtr = reinterpret_cast<IDirect3DDevice9**>(Game::NFS_D3D9_DEVICE_ADDRESS);
        if (core::IsReadable(devPtr, sizeof(void*)) && *devPtr)
        {
            m_device = *devPtr;
            OutputDebugStringA("[PrecipitationController::Update] m_device resolved from NFS_D3D9_DEVICE_ADDRESS\n");
        }
        else
        {
            OutputDebugStringA("[PrecipitationController::Update] m_device still null after resolve\n");
        }
    }
    if (!m_active)
    {
        static bool loggedInactive = false;
        if (!loggedInactive)
        {
            RainDebugOut("[PrecipitationController::Update] inactive\n");
            loggedInactive = true;
        }
        return;
    }

    static bool rngSeeded = false;
    if (!rngSeeded)
    {
        g_rngState ^= static_cast<uint32_t>(core::CurrentTime);
        rngSeeded = true;
    }

    if (!m_device)
    {
        RainDebugOut("[Update] 🚫 m_device still null, skipping update\n");
        return;
    }

    if (detected_game == GameType::MW)
    {
        if (g_precipitationConfig.applyPresetGlobals)
            ApplyNativePresetGlobalsMW();

        if (g_precipitationConfig.enable3DRain || g_precipitationConfig.enable3DSplatters)
        {
            auto* rainEnable = reinterpret_cast<int*>(Game::RainEnablePtr);
            auto* particleEnable = reinterpret_cast<int*>(Game::ParticleSystemEnablePtr);
            if (core::IsReadable(rainEnable, sizeof(int)))
                *rainEnable = 1;
            if (core::IsReadable(particleEnable, sizeof(int)))
                *particleEnable = 1;
        }

        // Force precipitation globals so Rain::Render path is active.
        if (g_precipitationConfig.enable3DRain || g_precipitationConfig.enable3DSplatters)
        {
            auto* precipEnable = reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR);
            if (core::IsReadable(precipEnable, sizeof(int)))
                *precipEnable = 1;

            auto* precipPercent = reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
            auto* rainPct = reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR);
            auto* fogPct = reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR);
            if (core::IsReadable(precipPercent, sizeof(float)))
                *precipPercent = 1.0f;
            // In independent flow, RainFlowMW drives these (with smoothing).
            // if (!kUseIndependentRainFlow)
            // {
            //     if (core::IsReadable(rainPct, sizeof(float)))
            //         *rainPct = g_precipitationConfig.rainIntensity;
            //     if (core::IsReadable(fogPct, sizeof(float)))
            //         *fogPct = g_precipitationConfig.fogIntensity;
            // }
        }

        PrecipitationController::ResetViewMatrix();
        D3DXMATRIX view{};
        D3DXMATRIX proj{};
        bool hasView = false;
        bool hasProj = false;
        void* camPtr = PrecipitationController::Get()->m_cameraPtr;
        if (!camPtr)
            camPtr = GetCameraPtrFromEView();
        if (camPtr)
            DebugLogCameraPtrOnce("Update", camPtr);
        if (g_precipitationConfig.preferHookedView)
        {
            if (PrecipitationController::GetViewMatrix(view) && !IsIdentityMatrix(view))
                hasView = true;
            else if (PrecipitationController::GetD3DViewProj(view, proj))
            {
                hasView = true;
                hasProj = true;
            }
        }
        if (!hasView && PrecipitationController::GetD3DViewProj(view, proj))
        {
            hasView = true;
            hasProj = true;
        }
        if (!hasView && GetEViewMatrix(view))
        {
            hasView = true;
            void* viewPtr = g_ActiveViewPtr;
            if (!viewPtr)
                viewPtr = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
            D3DVIEWPORT9 vp{};
            if (FAILED(m_device->GetViewport(&vp)))
                vp = {0, 0, 1920, 1080, 0.0f, 1.0f};
            void* camPtrNorm = NormalizeCameraPtr(camPtr);
            if (GetProjFromEView(viewPtr, vp, proj) ||
                GetProjFromCameraParams(camPtrNorm, vp, proj))
            {
                hasProj = true;
            }
            else if (PrecipitationController::GetD3DProj(proj))
            {
                hasProj = true;
            }
            else
            {
                float aspect = static_cast<float>(vp.Width) / static_cast<float>(vp.Height);
                D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(60.0f), aspect, 1.0f, 500.0f);
                hasProj = true;
                DebugLogMatrixSourceOnce("Update fallback proj");
                DebugLogMatrixNoGuard("Update fallback proj", proj);
            }
            if (IsProjectionLike(view))
            {
                if (!camPtr)
                    camPtr = GetCameraPtrFromEView();
                DebugLogCameraPtrDetailsOnce(camPtr);
                void* camNorm = NormalizeCameraPtr(camPtr);
                if (camNorm != camPtr)
                    DebugLogCameraPtrDetailsOnce2("cameraPtrNorm", camNorm);
                D3DXMATRIX camMat{};
                if (GetCameraMatrixFromPtr(camPtr, camMat))
                {
                    D3DXMATRIX inv{};
                    if (D3DXMatrixInverse(&inv, nullptr, &camMat))
                        view = inv;
                }
                else
                {
                    D3DXMATRIX viewFromParams{};
                    if (GetViewFromCameraParams(camPtr, viewFromParams))
                        view = viewFromParams;
                }
                if (GetViewFromCameraStruct(camPtr, view))
                {
                    DebugLogMatrixSourceOnce("Update cameraPtr struct view");
                    DebugLogMatrixNoGuard("Update cameraPtr struct", view);
                }
                if (GetViewFromCameraPtr(camPtr, view))
                {
                    DebugLogMatrixSourceOnce("Update cameraPtr view");
                    DebugLogMatrixNoGuard("Update cameraPtr", view);
                }
                void* viewPtr = g_ActiveViewPtr;
                if (!viewPtr)
                    viewPtr = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
                D3DVIEWPORT9 vp{};
                if (FAILED(m_device->GetViewport(&vp)))
                    vp = {0, 0, 1920, 1080, 0.0f, 1.0f};
                D3DXMATRIX viewFromParams{};
                D3DXMATRIX projFromParams{};
                void* camPtrNorm = NormalizeCameraPtr(camPtr);
                if (GetViewProjFromCameraParams(camPtrNorm, vp, viewFromParams, projFromParams))
                {
                    view = viewFromParams;
                    proj = projFromParams;
                    hasProj = true;
                    DebugLogMatrixSourceOnce("Update camera params view/proj");
                    DebugLogMatrixNoGuard("Update camView", view);
                }
            }
        }
        if (!hasView && g_precipitationConfig.useLookAtMatrix && PrecipitationController::GetViewMatrix(view))
        {
            hasView = true;
            hasProj = PrecipitationController::GetD3DProj(proj);
        }
        if (!hasView && GetRainMatrices(view, proj))
        {
            hasView = true;
            hasProj = true;
        }

        if (hasView)
        {
            static bool loggedUpdateMatrix = false;
            if (!loggedUpdateMatrix)
            {
                DebugLogMatrixSourceOnce("Update hasView");
                DebugLogMatrixNoGuard("Update view", view);
                if (hasProj)
                    DebugLogMatrixNoGuard("Update proj", proj);
                loggedUpdateMatrix = true;
            }
            // Always try eView projection in MW if available
            void* viewPtr = g_ActiveViewPtr;
            if (!viewPtr)
                viewPtr = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
            D3DVIEWPORT9 vp{};
            if (FAILED(m_device->GetViewport(&vp)))
                vp = {0, 0, 1920, 1080, 0.0f, 1.0f};
            if (GetProjFromEView(viewPtr, vp, proj))
                hasProj = true;
            if (camPtr)
            {
                void* camPtrNorm = NormalizeCameraPtr(camPtr);
                bool hasCamView = GetViewFromCameraStruct(camPtrNorm, view);
                if (!hasCamView)
                    hasCamView = GetViewFromCameraParams(camPtrNorm, view);
                if (hasCamView && GetProjFromCameraParams(camPtrNorm, vp, proj))
                {
                    hasProj = true;
                    DebugLogMatrixSourceOnce("Update camera params proj override");
                }
            }
            if (camPtr)
            {
                void* camPtrNorm = NormalizeCameraPtr(camPtr);
                void* camParams = GetCameraParamsFromEView();
                if (camParams)
                {
                    D3DXVECTOR3 pos{};
                    if (GetCameraPosFromCamParams(camParams, pos))
                        camPos = pos;
                }
                else if (camPtrNorm)
                {
                    D3DXVECTOR3 pos{};
                    if (GetCameraPosFromCamParams(camPtrNorm, pos))
                        camPos = pos;
                }
            }
            D3DXMATRIX invView{};
            if (D3DXMatrixInverse(&invView, nullptr, &view))
            {
                camPos = D3DXVECTOR3(invView._41, invView._42, invView._43);
                static bool loggedCamPosFromView = false;
                if (!loggedCamPosFromView)
                {
                    char buf[128];
                    sprintf_s(buf, "[RainDebug] camPosFromView=(%.2f,%.2f,%.2f)\n",
                              camPos.x, camPos.y, camPos.z);
                    RainDebugOut(buf);
                    loggedCamPosFromView = true;
                }
            }
        }
    }

    if (camPos == D3DXVECTOR3(0, 0, 0))
        camPos = GetCameraPositionSafe();

    static bool s_hasLastCamPos = false;
    static D3DXVECTOR3 s_lastCamPos{};
    if ((detected_game == GameType::MW || detected_game == GameType::CB))
    {
        if (s_hasLastCamPos)
        {
            D3DXVECTOR3 delta = camPos - s_lastCamPos;
            if (D3DXVec3LengthSq(&delta) > 1e-4f)
            {
                for (auto& drop : m_drops3D)
                    drop.position += delta;
                for (auto& splat : m_splatters3D)
                    splat.position += delta;
            }
        }
        s_lastCamPos = camPos;
        s_hasLastCamPos = true;
    }

    m_cameraY = GetUpCoord(camPos);

    if (IsCameraCovered(camPos))
        return;

    D3DVIEWPORT9 viewport{};
    if (FAILED(m_device->GetViewport(&viewport)))
        return;

    // MW native rain runs inside the game's render path; avoid polluting device state here.
    // When preset rendering is enabled (e.g., snow), we allow the custom renderer for MW.
    if (detected_game == GameType::MW &&
        (g_precipitationConfig.enable3DRain || g_precipitationConfig.enable3DSplatters) &&
        !g_precipitationConfig.applyPresetRendering)
        return;

    // Setup shared render state
    // m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_ALPHAREF, 32);
    m_device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);

    if (g_precipitationConfig.fpsOverride > 0.0f)
    {
        core::fpsDeltaTime = 1.0f / g_precipitationConfig.fpsOverride;
    }
    else
    {
        DWORD currentTime = core::CurrentTime;
        float rawDelta = (currentTime - m_lastTime) / 1000.0f;

        // Clamp to prevent stalling or oversimulation
        if (rawDelta < 0.001f)
            rawDelta = 0.001f; // Min cap (1000 FPS)
        else if (rawDelta > 0.1f)
            rawDelta = 0.016f; // Max cap (~60 FPS fallback)

        core::fpsDeltaTime = rawDelta;
        m_lastTime = currentTime;
    }

    if (g_precipitationConfig.enable3DRain)
    {
        if (!m_rainTex && !g_precipitationConfig.use_raindrop_dds)
            IsCreatedRainTexture();

        m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        m_device->SetTexture(0, m_rainTex);
        Render3DRainOverlay(viewport);
    }

    if (g_precipitationConfig.enable3DSplatters)
    {
        Render3DSplattersOverlay(viewport);
    }

    if (g_precipitationConfig.enable2DRain)
    {
        Render2DRainOverlay(viewport);
    }

    // m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}
