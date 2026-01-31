#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <cstring>
#include <memory>
#include <vector>
#include "Math.h"
#include "Game.h"
#include "features.h"
#include "core.h"
#include "PrecipitationController.h"
#include "RainConfigController.h"
#include "NFSMW_PreFEngHook.h"
#include "RainFlow/RainFlowMW.h"

#include "injector/injector.hpp"
#include <cstdio>
#include "minhook/include/MinHook.h"
#include "CPatch.h"
#include "GameAddresses.h"
// ==========================================================
// DLL ENTRY
// ==========================================================
GameType detected_game = GameType::Unknown;

using namespace ngg::common;

static std::vector<std::unique_ptr<ngg::common::Feature>> g_features;
static bool triedInit = false;

using CreateLookAt_t = void(__cdecl*)(Mat4* mat, Vec3* eye, Vec3* center, Vec3* up);
static CreateLookAt_t g_originalCreateLookAt = nullptr;
using BuildView_t = int(__fastcall*)(void* viewObj, void* edx, int a0, float a4, float a8, int aC, char a10);
static BuildView_t g_originalBuildView = nullptr;
using BuildRenderView_t = int(__cdecl*)(void* a0, int a4, int a8);
static BuildRenderView_t g_originalBuildRenderView = nullptr;
using BuildRenderMatrix_t = void(__cdecl*)(void* outMat, void* inMat);
static BuildRenderMatrix_t g_originalBuildRenderMatrix = nullptr;
using DisplayFrame_t = void(__cdecl*)();
static DisplayFrame_t g_originalDisplayFrame = nullptr;

static IDirect3DDevice9* GetGameDevice()
{
    if (detected_game != GameType::MW)
        return nullptr;

    auto** devicePtr = reinterpret_cast<IDirect3DDevice9**>(Game::NFS_D3D9_DEVICE_ADDRESS);
    if (!core::IsReadable(devicePtr, sizeof(void*)))
        return nullptr;
    return *devicePtr;
}

static void __fastcall HookedRainUpdateCallsite(void* ecx, void*);
static void __fastcall HookedRainRenderCallsite(void* ecx, void*);
static void HookedRenderCtxCallsite();
static void AfterRenderCtxCallsite();
static constexpr bool kUseIndependentRainFlow = true;

static void RunNativeRainFlow(void* rain)
{
    if (!rain || !core::IsReadable(rain, 0x400))
        return;

    static bool entryLogged = false;
    if (!entryLogged)
    {
        OutputDebugStringA("[WeatherMod] RunNativeRainFlow entered\n");
        entryLogged = true;
    }

    void* viewPtr = nullptr;
    void* viewPlat = nullptr;
    static bool viewScanLogged = false;
    if (Game::EViewArrayBase && Game::EViewSize && Game::EViewArrayCount)
    {
        for (uintptr_t i = 0; i < Game::EViewArrayCount; ++i)
        {
            uintptr_t entry = Game::EViewArrayBase + i * Game::EViewSize;
            if (!core::IsReadable(reinterpret_cast<void*>(entry), Game::EViewActiveFlagOffset + 1))
                continue;
            void* candidateView = reinterpret_cast<void*>(entry);
            if (!core::IsReadable(candidateView, 0x50))
                continue;
            void* candidatePlat = *reinterpret_cast<void**>(entry + 0x44);
            if (candidatePlat)
            {
                if (!viewScanLogged)
                {
                    char dbg[256];
                    unsigned char active = *reinterpret_cast<unsigned char*>(entry + Game::EViewActiveFlagOffset);
                    std::snprintf(dbg, sizeof(dbg),
                                  "[WeatherMod] eView[%u]=0x%p active=%u viewPlat=0x%p\n",
                                  static_cast<unsigned>(i), candidateView, active, candidatePlat);
                    OutputDebugStringA(dbg);
                }
                viewPtr = *reinterpret_cast<void**>(entry);
                viewPlat = candidatePlat;
                break;
            }
        }
        viewScanLogged = true;
    }
    if (!viewPtr && core::IsReadable(reinterpret_cast<void*>(Game::EViewCurrentPtr), sizeof(void*)))
    {
        void* current = *reinterpret_cast<void**>(Game::EViewCurrentPtr);
        if (current != reinterpret_cast<void*>(Game::EViewArrayBase) && core::IsReadable(current, 0x50))
        {
            void* currentPlat = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(current) + 0x44);
            if (currentPlat)
            {
                viewPtr = current;
                viewPlat = currentPlat;
            }
        }
    }
    static bool stateLogged = false;

    if (viewPlat && core::IsReadable(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rain) + 0x288), sizeof(void*)))
    {
        *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x284) = viewPlat;
        *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x288) = viewPtr;
    }
    void* p284 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x284);
    void* p288 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x288);
    if (!stateLogged)
    {
        char dbg[256];
        std::snprintf(dbg, sizeof(dbg),
                      "[WeatherMod] rain=0x%p view=0x%p viewPlat=0x%p p284=0x%p p288=0x%p\n",
                      rain, viewPtr, viewPlat, p284, p288);
        OutputDebugStringA(dbg);
        stateLogged = true;
    }
    if (!p284 || !p288)
    {
        static bool logged = false;
        if (!logged)
        {
            OutputDebugStringA("[WeatherMod] early-exit: p284/p288 null\n");
            logged = true;
        }
        return;
    }
    if (p288 == reinterpret_cast<void*>(Game::EViewArrayBase))
    {
        static bool logged = false;
        if (!logged)
        {
            OutputDebugStringA("[WeatherMod] early-exit: p288 is array base\n");
            logged = true;
        }
        return;
    }

    if (!core::IsReadable(p284, sizeof(void*)) ||
        !core::IsReadable(p288, 0x50))
    {
        static bool logged = false;
        if (!logged)
        {
            OutputDebugStringA("[WeatherMod] early-exit: p284/p288 unreadable\n");
            logged = true;
        }
        return;
    }

    void* viewPlatVtable = *reinterpret_cast<void**>(p284);
    if (!core::IsReadable(viewPlatVtable, sizeof(void*)))
    {
        static bool logged = false;
        if (!logged)
        {
            OutputDebugStringA("[WeatherMod] early-exit: viewPlat vtable unreadable\n");
            logged = true;
        }
        return;
    }

    if (!core::IsReadable(reinterpret_cast<void*>(Game::renderCtxAddr), sizeof(void*)))
        return;
    auto* renderCtx = reinterpret_cast<void*>(Game::renderCtxAddr);
    if (!*reinterpret_cast<void**>(renderCtx))
    {
        // Prefer render platform pointer if available, otherwise fall back to particle ctx.
        auto* renderPlat = reinterpret_cast<void*>(Game::renderPlatAddr);
        if (core::IsReadable(renderPlat, sizeof(void*)) && *reinterpret_cast<void**>(renderPlat))
        {
            *reinterpret_cast<void**>(renderCtx) = *reinterpret_cast<void**>(renderPlat);
        }
        else
        {
            auto* particleCtx = reinterpret_cast<void*>(Game::particleCtxAddr);
            if (core::IsReadable(particleCtx, sizeof(void*)) && *reinterpret_cast<void**>(particleCtx))
            {
                *reinterpret_cast<void**>(renderCtx) = *reinterpret_cast<void**>(particleCtx);
            }
            else
            {
                return;
            }
        }
    }
    void* ctxVal = *reinterpret_cast<void**>(renderCtx);
    if (!core::IsReadable(ctxVal, sizeof(void*)))
        return;

    bool enabled = PrecipitationController::Get()->IsActive();
    if (!enabled)
    {
        auto* rainEnable = reinterpret_cast<int*>(Game::RainEnablePtr);
        auto* particleEnable = reinterpret_cast<int*>(Game::ParticleSystemEnablePtr);
        auto* precipEnable = reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR);
        auto* precipRender = reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR);

        if (core::IsReadable(rainEnable, sizeof(int)))
            *rainEnable = 0;
        if (core::IsReadable(particleEnable, sizeof(int)))
            *particleEnable = 0;
        if (core::IsReadable(precipEnable, sizeof(int)))
            *precipEnable = 0;
        if (core::IsReadable(precipRender, sizeof(int)))
            *precipRender = 0;

        auto* precipPercent = reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
        auto* rainPercent = reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR);
        auto* fogPercent = reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR);
        auto* roadReflection = reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR);
        if (core::IsReadable(precipPercent, sizeof(float)))
            *precipPercent = 0.0f;
        if (core::IsReadable(rainPercent, sizeof(float)))
            *rainPercent = 0.0f;
        if (core::IsReadable(fogPercent, sizeof(float)))
            *fogPercent = 0.0f;
        if (core::IsReadable(roadReflection, sizeof(float)))
            *roadReflection = 0.0f;
        OutputDebugStringA("[WeatherMod] early-exit: disabled\n");
        return;
    }

    auto* rainEnable = reinterpret_cast<int*>(Game::RainEnablePtr);
    auto* particleEnable = reinterpret_cast<int*>(Game::ParticleSystemEnablePtr);
    auto* precipEnable = reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR);
    auto* precipRender = reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR);

    if (core::IsReadable(rainEnable, sizeof(int)))
        *rainEnable = 1;
    if (core::IsReadable(particleEnable, sizeof(int)))
        *particleEnable = 1;
    if (core::IsReadable(precipEnable, sizeof(int)))
        *precipEnable = 1;
    if (core::IsReadable(precipRender, sizeof(int)))
        *precipRender = 1;

    static bool loggedOnce = false;
    float rainPct = RainConfigController::precipitationConfig.rainIntensity;
    if (rainPct < 0.0f)
        rainPct = 0.0f;
    float fogPct = RainConfigController::precipitationConfig.fogIntensity;
    if (fogPct < 0.0f)
        fogPct = 0.0f;
    if (!loggedOnce)
    {
        char dbg[256];
        std::snprintf(dbg, sizeof(dbg),
                      "[WeatherMod] cfg rain=%.3f fog=%.3f enable3DRain=%d enable3DSplatters=%d\n",
                      rainPct, fogPct,
                      RainConfigController::precipitationConfig.enable3DRain ? 1 : 0,
                      RainConfigController::precipitationConfig.enable3DSplatters ? 1 : 0);
        OutputDebugStringA(dbg);
        loggedOnce = true;
    }

    if (Game::g_originalRainSetOverrideIntensity)
        Game::g_originalRainSetOverrideIntensity(rainPct);
    if (Game::g_originalRainSetIntensity)
        Game::g_originalRainSetIntensity(rain, rainPct);
    if (Game::g_originalGameSetChanceOfRain)
        Game::g_originalGameSetChanceOfRain(rainPct);

    // Force weather param-map rain layer intensity (MW).
    auto* paramMapRain = reinterpret_cast<float*>(Game::kParamMapLayerRain);
    auto* paramDataRain = reinterpret_cast<float*>(Game::kParamDataRain);
    if (core::IsReadable(paramMapRain, sizeof(float)))
        *paramMapRain = rainPct;
    if (core::IsReadable(paramDataRain, sizeof(float)))
        *paramDataRain = rainPct;

    auto* precipPercent = reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
    auto* rainPercent = reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR);
    auto* fogPercent = reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR);
    if (core::IsReadable(precipPercent, sizeof(float)))
        *precipPercent = rainPct;
    if (core::IsReadable(rainPercent, sizeof(float)))
        *rainPercent = rainPct;
    if (core::IsReadable(fogPercent, sizeof(float)))
        *fogPercent = fogPct;

    // Native rain tuning globals (from IDA)
    auto* generalRainAmount = reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
    auto* rainCrossing = reinterpret_cast<float*>(Game::PRECIP_RAINY_ADDR);
    auto* rainFallSpeed = reinterpret_cast<float*>(Game::PRECIP_RAINZ_ADDR);
    auto* rainGravity = reinterpret_cast<float*>(Game::PRECIP_RAINZCONSTANT_ADDR);
    auto* fallingRainSize = reinterpret_cast<float*>(Game::PRECIP_RAINRADIUSX_ADDR);
    auto* rainIntensity = reinterpret_cast<float*>(Game::PRECIP_RAINRADIUSY_ADDR);
    auto* roadReflection = reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR);

    if (core::IsReadable(generalRainAmount, sizeof(float)))
        *generalRainAmount = rainPct;
    if (core::IsReadable(rainIntensity, sizeof(float)))
        *rainIntensity = rainPct;
    if (core::IsReadable(roadReflection, sizeof(float)))
        *roadReflection = rainPct;
    // Keep these at defaults unless you wire config later.
    if (core::IsReadable(rainCrossing, sizeof(float)))
        *rainCrossing = 0.02f;
    if (core::IsReadable(rainFallSpeed, sizeof(float)))
        *rainFallSpeed = 0.03f;
    if (core::IsReadable(rainGravity, sizeof(float)))
        *rainGravity = 0.35f;
    if (core::IsReadable(fallingRainSize, sizeof(float)))
        *fallingRainSize = 0.01f;

    // Ensure Rain instance intensity targets are non-zero.
    if (core::IsReadable(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(rain) + 0x290), sizeof(float)))
    {
        *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(rain) + 0x28C) = rainPct;
        *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(rain) + 0x290) = rainPct;
    }

    D3DXVECTOR3 camPos = PrecipitationController::Get()->GetCameraPositionSafe();
    auto* rainX = reinterpret_cast<float*>(Game::PRECIP_RAINX_ADDR);
    auto* rainY = reinterpret_cast<float*>(Game::PRECIP_RAINY_ADDR);
    auto* rainZ = reinterpret_cast<float*>(Game::PRECIP_RAINZ_ADDR);
    if (core::IsReadable(rainX, sizeof(float)) &&
        core::IsReadable(rainY, sizeof(float)) &&
        core::IsReadable(rainZ, sizeof(float)))
    {
        *rainX = camPos.x;
        *rainY = camPos.y;
        *rainZ = camPos.z;
    }

    // Use Rain::Update instead of wrapper tick.
    // Update is handled via the native callsite hook.

    // Force native render in the same context (tick alone doesn't draw).
    static bool renderLogged = false;
    if (!renderLogged)
    {
        char dbg[160];
        std::snprintf(dbg, sizeof(dbg), "[WeatherMod] renderCtx=0x%p\n", ctxVal);
        OutputDebugStringA(dbg);
        renderLogged = true;
    }
    // Leave update/render to the native wrapper (called at safe time).
}


static bool IsLikelyRainInstance(void* rain)
{
    if (!rain || !core::IsReadable(rain, 0x400))
        return false;
    uintptr_t r = reinterpret_cast<uintptr_t>(rain);
    if (Game::EViewArrayBase && Game::EViewSize && Game::EViewArrayCount)
    {
        uintptr_t end = Game::EViewArrayBase + Game::EViewSize * Game::EViewArrayCount;
        if (r >= Game::EViewArrayBase && r < end)
            return false;
    }
    void* p284 = *reinterpret_cast<void**>(r + 0x284);
    void* p288 = *reinterpret_cast<void**>(r + 0x288);
    if (!p284 || !p288)
        return false;
    return true;
}

static void __fastcall HookedRainTickCallsite(void* ecx, void*)
{
    static bool logged = false;
    static bool triedInit = false;
    void* rain = ecx;
    if (!IsLikelyRainInstance(rain))
    {
        rain = *reinterpret_cast<void**>(Game::RainInstancePtr);
        if (!IsLikelyRainInstance(rain))
        {
            if (!triedInit && PrecipitationController::Get()->IsActive() && Game::g_originalInitViews)
            {
                OutputDebugStringA("[WeatherMod] Rain instance missing, calling epInitViews\n");
                Game::g_originalInitViews();
                triedInit = true;
                rain = *reinterpret_cast<void**>(Game::RainInstancePtr);
            }
            static bool missingLogged = false;
            if (!missingLogged)
            {
                char dbg[256];
                if (rain && core::IsReadable(rain, 0x300))
                {
                    void* p284 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x284);
                    void* p288 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x288);
                    float f28c = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(rain) + 0x28C);
                    float f290 = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(rain) + 0x290);
                    std::snprintf(dbg, sizeof(dbg),
                                  "[WeatherMod] Rain instance missing (ecx=0x%p, ptr=0x%p) p284=0x%p p288=0x%p f28c=%.3f f290=%.3f\n",
                                  ecx, rain, p284, p288, f28c, f290);
                }
                else
                {
                    std::snprintf(dbg, sizeof(dbg),
                                  "[WeatherMod] Rain instance missing (ecx=0x%p, ptr=0x%p)\n",
                                  ecx, rain);
                }
                OutputDebugStringA(dbg);
                missingLogged = true;
            }

            if (PrecipitationController::Get()->IsActive())
            {
                auto* rainEnable = reinterpret_cast<int*>(Game::RainEnablePtr);
                auto* particleEnable = reinterpret_cast<int*>(Game::ParticleSystemEnablePtr);
                auto* precipEnable = reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR);
                auto* precipRender = reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR);
                if (core::IsReadable(rainEnable, sizeof(int)))
                    *rainEnable = 1;
                if (core::IsReadable(particleEnable, sizeof(int)))
                    *particleEnable = 1;
                if (core::IsReadable(precipEnable, sizeof(int)))
                    *precipEnable = 1;
                if (core::IsReadable(precipRender, sizeof(int)))
                    *precipRender = 1;

                float rainPct = RainConfigController::precipitationConfig.rainIntensity;
                if (rainPct < 0.0f)
                    rainPct = 0.0f;
                float fogPct = RainConfigController::precipitationConfig.fogIntensity;
                if (fogPct < 0.0f)
                    fogPct = 0.0f;

                auto* precipPercent = reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
                auto* rainPercent = reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR);
                auto* fogPercent = reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR);
                if (core::IsReadable(precipPercent, sizeof(float)))
                    *precipPercent = rainPct;
                if (core::IsReadable(rainPercent, sizeof(float)))
                    *rainPercent = rainPct;
                if (core::IsReadable(fogPercent, sizeof(float)))
                    *fogPercent = fogPct;

                auto* generalRainAmount = reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
                auto* rainIntensity = reinterpret_cast<float*>(Game::PRECIP_RAINRADIUSY_ADDR);
                auto* roadReflection = reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR);
                if (core::IsReadable(generalRainAmount, sizeof(float)))
                    *generalRainAmount = rainPct;
                if (core::IsReadable(rainIntensity, sizeof(float)))
                    *rainIntensity = rainPct;
                if (core::IsReadable(roadReflection, sizeof(float)))
                    *roadReflection = rainPct;

                D3DXVECTOR3 camPos = PrecipitationController::Get()->GetCameraPositionSafe();
                auto* rainX = reinterpret_cast<float*>(Game::PRECIP_RAINX_ADDR);
                auto* rainY = reinterpret_cast<float*>(Game::PRECIP_RAINY_ADDR);
                auto* rainZ = reinterpret_cast<float*>(Game::PRECIP_RAINZ_ADDR);
                if (core::IsReadable(rainX, sizeof(float)) &&
                    core::IsReadable(rainY, sizeof(float)) &&
                    core::IsReadable(rainZ, sizeof(float)))
                {
                    *rainX = camPos.x;
                    *rainY = camPos.y;
                    *rainZ = camPos.z;
                }
            }
            return;
        }
    }
    if (!logged)
    {
        OutputDebugStringA("[RainCallsite] HookedRainTickCallsite hit\n");
        logged = true;
    }
    if (!kUseIndependentRainFlow)
        RunNativeRainFlow(rain);
}

static void __fastcall HookedRainUpdateCallsite(void* ecx, void*)
{
    if (kUseIndependentRainFlow)
        return;
    static bool logged = false;
    void* rain = ecx;
    if (!IsLikelyRainInstance(rain))
        return;
    if (!logged)
    {
        OutputDebugStringA("[WeatherMod] HookedRainUpdateCallsite hit\n");
        logged = true;
    }
    // Apply globals and wire view pointers before the engine update runs.
    RunNativeRainFlow(rain);
    if (Game::g_originalRainUpdate)
        Game::g_originalRainUpdate(rain);
}

static void __fastcall HookedRainRenderCallsite(void* ecx, void*)
{
    if (kUseIndependentRainFlow)
        return;
    static bool logged = false;
    void* rain = ecx;
    if (!IsLikelyRainInstance(rain))
        return;
    if (!logged)
    {
        OutputDebugStringA("[WeatherMod] HookedRainRenderCallsite hit\n");
        logged = true;
    }
    // Ensure globals are applied before the engine render.
    RunNativeRainFlow(rain);
    if (Game::g_originalRainRender)
        Game::g_originalRainRender(rain);
}

static void AfterRenderCtxCallsite()
{
    if (kUseIndependentRainFlow)
        return;
    static bool logged = false;
    if (!logged)
    {
        OutputDebugStringA("[WeatherMod] AfterRenderCtxCallsite hit\n");
        logged = true;
    }
    if (detected_game != GameType::MW)
        return;
    if (!PrecipitationController::Get()->IsActive())
        return;
    void* rain = *reinterpret_cast<void**>(Game::RainInstancePtr);
    if (IsLikelyRainInstance(rain))
        RunNativeRainFlow(rain);
}

static void __declspec(naked) HookedRenderCtxCallsite()
{
    if (kUseIndependentRainFlow)
    {
        __asm { ret }
    }
    __asm
    {
        // Call original vtable function at [ecx]+4
        pushad
        pushfd
        call AfterRenderCtxCallsite
        popfd
        popad
        pushad
        mov eax, [ecx]
        mov eax, [eax+4]
        mov edx, eax
        popad
        call edx
        ret
    }
}

static void __declspec(naked) HookedRenderCtxCallsite2()
{
    if (kUseIndependentRainFlow)
    {
        __asm { ret }
    }
    __asm
    {
        // Original: call dword ptr [ecx+15Ch] with two args already on stack.
        pushad
        pushfd
        call AfterRenderCtxCallsite
        popfd
        popad
        mov eax, [ecx+15Ch]
        call eax
        ret
    }
}

static void __fastcall HookedRainTick(void* ecx, void* edx)
{
    (void)edx;
    static bool logged = false;
    if (RainConfigController::precipitationConfig.enable3DRain ||
        RainConfigController::precipitationConfig.enable3DSplatters)
    {
        return;
    }
    if (detected_game == GameType::MW)
    {
        if (RainConfigController::precipitationConfig.enable3DRain ||
            RainConfigController::precipitationConfig.enable3DSplatters)
        {
            auto* rainEnable = reinterpret_cast<int*>(Game::RainEnablePtr);
            auto* particleEnable = reinterpret_cast<int*>(Game::ParticleSystemEnablePtr);
            if (core::IsReadable(rainEnable, sizeof(int)))
                *rainEnable = 1;
            if (core::IsReadable(particleEnable, sizeof(int)))
                *particleEnable = 1;

            auto* precipEnable = reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR);
            if (core::IsReadable(precipEnable, sizeof(int)))
                *precipEnable = 1;

            auto* precipPercent = reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
            if (core::IsReadable(precipPercent, sizeof(float)))
                *precipPercent = 1.0f;

            // Force rain intensity fields on the Rain instance.
            if (core::IsReadable(ecx, 0x294))
            {
                *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(ecx) + 0x28C) = 1.0f; // rain intensity
                *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(ecx) + 0x290) = 1.0f; // rain target
            }
        }
    }
    if (!logged)
    {
        OutputDebugStringA("[RainDebug] HookedRainTick called\n");
        logged = true;
    }
    // Avoid running tick when render context is not set (dword_982C80 == 0).
    auto* renderCtx = reinterpret_cast<void*>(Game::renderCtxAddr);
    if (!core::IsReadable(renderCtx, sizeof(void*)) || !*reinterpret_cast<void**>(renderCtx))
        return;
    if (Game::g_originalRainTick)
        Game::g_originalRainTick(ecx);
}

static void __fastcall HookedRainRender(void* ecx, void* edx)
{
    (void)edx;
    static bool logged = false;
    if (RainConfigController::precipitationConfig.enable3DRain ||
        RainConfigController::precipitationConfig.enable3DSplatters)
    {
        return;
    }
    if (!logged)
    {
        char buf[256];
        int precipEnable = 0;
        int precipRender = 0;
        float precipPercent = 0.0f;
        float rainPercent = 0.0f;
        int gameFlow = 0;
        int gameFlowStatus = 0;
        if (core::IsReadable(reinterpret_cast<void*>(Game::PRECIPITATION_ENABLE_ADDR), sizeof(int)))
            precipEnable = *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(Game::PRECIPITATION_RENDER_ADDR), sizeof(int)))
            precipRender = *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(Game::PRECIPITATION_PERCENT_ADDR), sizeof(float)))
            precipPercent = *reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINPERCENT_ADDR), sizeof(float)))
            rainPercent = *reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(Game::GAMEFLOWMGR_ADDR), sizeof(int)))
            gameFlow = *reinterpret_cast<int*>(Game::GAMEFLOWMGR_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(Game::GAMEFLOWMGR_STATUS_ADDR), sizeof(int)))
            gameFlowStatus = *reinterpret_cast<int*>(Game::GAMEFLOWMGR_STATUS_ADDR);
        std::snprintf(buf, sizeof(buf),
                      "[RainDebug] HookedRainRender called precipEnable=%d precipRender=%d precipPct=%.2f rainPct=%.2f gameFlow=%d gameFlowStatus=%d\n",
                      precipEnable, precipRender, precipPercent, rainPercent, gameFlow, gameFlowStatus);
        OutputDebugStringA(buf);

        if (core::IsReadable(ecx, 0x290))
        {
            void* p284 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ecx) + 0x284);
            void* p288 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ecx) + 0x288);
            std::snprintf(buf, sizeof(buf),
                          "[RainDebug] Rain instance ptr=0x%p p284=0x%p p288=0x%p\n",
                          ecx, p284, p288);
            OutputDebugStringA(buf);

            if (!p284 && p288 && core::IsReadable(p288, 0x70))
            {
                void* viewPlat = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(p288) + 0x44);
                if (viewPlat)
                {
                    *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ecx) + 0x284) = viewPlat;
                    OutputDebugStringA("[RainDebug] Patched Rain::mPtr284 = *(eView+0x44) (viewPlat)\n");
                }
            }
        }
        logged = true;
    }
    // Avoid crashing when render context is not set (dword_982C80 == 0).
    auto* renderCtx = reinterpret_cast<void*>(Game::renderCtxAddr);
    void* ctxVal = nullptr;
    if (core::IsReadable(renderCtx, sizeof(void*)))
        ctxVal = *reinterpret_cast<void**>(renderCtx);

    // If render context is missing, try to seed it from particle system context.
    if (!ctxVal)
    {
        auto* particleCtx = reinterpret_cast<void*>(Game::particleCtxAddr);
        if (core::IsReadable(particleCtx, sizeof(void*)) && *reinterpret_cast<void**>(particleCtx))
        {
            *reinterpret_cast<void**>(renderCtx) = *reinterpret_cast<void**>(particleCtx);
            ctxVal = *reinterpret_cast<void**>(renderCtx);
        }
    }

    if (ctxVal && Game::g_originalRainRender)
        Game::g_originalRainRender(ecx);
}

static void __fastcall HookedRainRender3D(void* ecx, void*)
{
    if (Game::g_originalRainRender3D)
        Game::g_originalRainRender3D(ecx);
}

// Forward declarations for per-frame logic
static void OnFrameUpdate();
static void HandleRainToggle();

// Helper: keep engine rain flags set so the native callsite runs.
static DWORD WINAPI RainGuardWorker(void*)
{
    while (true)
    {
        if (detected_game != GameType::MW)
            continue;

        OnFrameUpdate();
        HandleRainToggle();

        __try
        {
            const bool enabled = PrecipitationController::Get()->IsActive();
            if (enabled)
            {
                if (kUseIndependentRainFlow)
                    RainFlowMW::Tick();
                // Force rain/particle globals so the engine doesn't skip the block.
                *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR) = 1;
                *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR) = 1;
                *reinterpret_cast<int*>(Game::RainEnablePtr) = 1;
                *reinterpret_cast<int*>(Game::ParticleSystemEnablePtr) = 1;

                // Ensure particle context pointer is valid for sub_6DE300 path.
                void** particleCtx = reinterpret_cast<void**>(Game::particleCtxAddr);
                void** renderCtx = reinterpret_cast<void**>(Game::renderCtxAddr);
                if (core::IsReadable(particleCtx, sizeof(void*)) && *particleCtx)
                {
                    if (core::IsReadable(renderCtx, sizeof(void*)) && !*renderCtx)
                        *renderCtx = *particleCtx;
                }
            }
            else
            {
                if (kUseIndependentRainFlow)
                    RainFlowMW::Disable();
                *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR) = 0;
                *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR) = 0;
                *reinterpret_cast<int*>(Game::RainEnablePtr) = 0;
                *reinterpret_cast<int*>(Game::ParticleSystemEnablePtr) = 0;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            OutputDebugStringA("[RainGuard] Exception in guard worker\n");
        }
    }
    return 0;
}

static void __cdecl HookedDisplayFrame()
{
    if (g_originalDisplayFrame)
        g_originalDisplayFrame();
    if (detected_game != GameType::MW)
        return;
    if (!PrecipitationController::Get()->IsActive())
        return;
    void* rain = *reinterpret_cast<void**>(Game::RainInstancePtr);
    if (!IsLikelyRainInstance(rain))
        return;
    if (!core::IsReadable(reinterpret_cast<void*>(Game::renderCtxAddr), sizeof(void*)))
        return;
    void* ctxVal = *reinterpret_cast<void**>(Game::renderCtxAddr);
    if (!core::IsReadable(ctxVal, sizeof(void*)))
        return;
    // Reject common garbage pointer patterns (float 1.0f as pointer).
    if (ctxVal == reinterpret_cast<void*>(0x3F800000))
        return;
    void* p284 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x284);
    void* p288 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(rain) + 0x288);
    if (!p284 || !p288 || !core::IsReadable(p284, sizeof(void*)) || !core::IsReadable(p288, 0x50))
        return;
    // Wrapper call disabled; crashes inside 0x73CDCA.
}

static void __cdecl HookedCreateLookAt(Mat4* mat, Vec3* eye, Vec3* center, Vec3* up)
{
    static bool logged = false;
    if (g_originalCreateLookAt)
        g_originalCreateLookAt(mat, eye, center, up);

    if (mat)
    {
        D3DXMATRIX view{};
        std::memcpy(&view, mat, sizeof(D3DXMATRIX));
        PrecipitationController::UpdateViewMatrix(view);
        if (!logged)
        {
            OutputDebugStringA("[RainDebug] HookedCreateLookAt fired\n");
            logged = true;
        }
    }
}

static int __fastcall HookedBuildView(void* viewObj, void* edx, int a0, float a4, float a8, int aC, char a10)
{
    int result = g_originalBuildView ? g_originalBuildView(viewObj, edx, a0, a4, a8, aC, a10) : 0;
    if (viewObj && core::IsReadable(viewObj, 0x80 + sizeof(D3DXMATRIX)))
    {
        D3DXMATRIX view{};
        std::memcpy(&view, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(viewObj) + 0x80),
                    sizeof(D3DXMATRIX));
        PrecipitationController::UpdateViewMatrix(view);
    }
    return result;
}

static int __cdecl HookedBuildRenderView(void* a0, int a4, int a8)
{
    if (a0 && core::IsReadable(a0, 8))
    {
        int viewIndex = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(a0) + 4);
        if (viewIndex >= 0 && viewIndex < static_cast<int>(Game::EViewArrayCount))
        {
            uintptr_t entry = Game::EViewArrayBase +
                static_cast<uintptr_t>(viewIndex) * Game::EViewSize;
            if (core::IsReadable(reinterpret_cast<void*>(entry), Game::EViewCameraOffset + sizeof(void*)))
                PrecipitationController::UpdateActiveViewPtr(reinterpret_cast<void*>(entry));
        }
    }
    return g_originalBuildRenderView ? g_originalBuildRenderView(a0, a4, a8) : 0;
}

static void __cdecl HookedBuildRenderMatrix(void* outMat, void* inMat)
{
    static bool logged = false;
    if (g_originalBuildRenderMatrix)
        g_originalBuildRenderMatrix(outMat, inMat);

    if (outMat && core::IsReadable(outMat, sizeof(D3DXMATRIX)))
    {
        D3DXMATRIX view{};
        std::memcpy(&view, outMat, sizeof(D3DXMATRIX));
        PrecipitationController::UpdateViewMatrix(view);
        if (!logged)
        {
            OutputDebugStringA("[RainDebug] HookedBuildRenderMatrix fired\n");
            logged = true;
        }
    }
}

static bool InitMinHook()
{
    MH_STATUS status = MH_Initialize();
    return status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED;
}

static void SetupFeatures()
{
    g_features.emplace_back(std::make_unique<PrecipitationController>());
}

static void InitializeWeather()
{
    if (g_features.empty())
        SetupFeatures();

    OutputDebugStringA("[InitializeWeather] Setting up hooks\n");
    RainConfigController::LoadOnStartup();
    RainConfigController::Load();

    for (const auto& feature : g_features)
    {
        const char* name = feature->name();
        OutputDebugStringA(name);
        OutputDebugStringA(" initialized\n");

        if (strcmp(name, "PrecipitationController") == 0)
        {
            auto* controller = PrecipitationController::Get();

            if (RainConfigController::precipitationConfig.enableOnStartup)
            {
                controller->DebugEVIEWListPtr();

                if (!controller->m_rainTex && !RainConfigController::precipitationConfig.use_raindrop_dds)
                    controller->IsCreatedRainTexture();

                OutputDebugStringA("[InitializeWeather] Precipitation configured for startup, waiting for camera...\n");
            }
            else
            {
                controller->disable();
                OutputDebugStringA("[InitializeWeather] Precipitation disabled in config\n");
            }
        }
        else
        {
            feature->enable();
        }
    }
}

static bool IsWeatherInitReady()
{
    uintptr_t feAddr = Game::FEMANAGER_INSTANCE_ADDR;
    if (feAddr)
    {
        auto* feManager = *reinterpret_cast<void**>(feAddr);
        if (feManager && core::IsReadable(feManager, 0x40))
            return true;
    }

    IDirect3DDevice9* device = PrecipitationController::Get()->m_device;
    if (!device)
        return false;

    D3DXMATRIX view{};
    return SUCCEEDED(device->GetTransform(D3DTS_VIEW, &view));
}

static void OnFrameUpdate()
{
    if (triedInit)
        return;

    if (IsWeatherInitReady())
    {
        triedInit = true;
        InitializeWeather();
    }
}

static void HandleRainToggle()
{
    static bool lastKeyState = false;
    static bool rainEnabled = false;
    static bool shouldEnable = false;
    static bool alreadyWarned = false;
    static bool skipOneToggle = false;

    bool keyPressed = (GetAsyncKeyState(RainConfigController::toggleKey) & 0x8000) != 0;

    if (keyPressed && !lastKeyState)
    {
        if (skipOneToggle)
        {
            skipOneToggle = false;
            lastKeyState = keyPressed;
            return;
        }

        shouldEnable = !shouldEnable;

        if (shouldEnable)
        {
            OutputDebugStringA("[RainToggle] Waiting for camera to become valid...\n");
            alreadyWarned = false;
        }
        else
        {
            if (rainEnabled)
            {
                PrecipitationController::Get()->disable();
                OutputDebugStringA("[RainToggle] Rain disabled by user\n");
                rainEnabled = false;
            }
            alreadyWarned = false;
        }
    }

    if (shouldEnable && !rainEnabled)
    {
        if (detected_game == GameType::MW)
        {
            if (!PrecipitationController::Get()->IsActive())
            {
                PrecipitationController::Get()->enable();
                OutputDebugStringA("[RainToggle] Rain enabled\n");
            }
            rainEnabled = PrecipitationController::Get()->IsActive();
            lastKeyState = keyPressed;
            return;
        }

        bool canEnable = false;
        D3DXVECTOR3 cam = PrecipitationController::Get()->GetCameraPositionSafe();
        if (cam != D3DXVECTOR3(0, 0, 0))
            canEnable = true;

        if (canEnable)
        {
            if (!PrecipitationController::Get()->IsActive())
            {
                PrecipitationController::Get()->enable();
                OutputDebugStringA("[RainToggle] Camera valid, rain enabled\n");
            }
            rainEnabled = PrecipitationController::Get()->IsActive();
        }
        else if (!alreadyWarned)
        {
            OutputDebugStringA("[RainToggle] Still waiting for camera...\n");
            alreadyWarned = true;
        }
    }

    if (!shouldEnable && !rainEnabled && RainConfigController::precipitationConfig.enableOnStartup)
    {
        shouldEnable = true;
    }

    lastKeyState = keyPressed;
}

DWORD WINAPI MainThread(void*)
{
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
    auto* dos = (IMAGE_DOS_HEADER*)base;
    auto* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

    uintptr_t entry = base + nt->OptionalHeader.AddressOfEntryPoint;
    switch (entry)
    {
    case 0x7C4040: detected_game = GameType::MW;
        {
            Game::Init(detected_game);
            break;
        }
    case 0x87E926: detected_game = GameType::CB;
        {
            Game::Init(detected_game);
            break;
        }
    case 0x75BCC7: detected_game = GameType::UG2;
        break;
    case 0x670CB5: detected_game = GameType::UG;
        break;
    }

    if (strstr((const char*)(base + (0xA49742 - 0x400000)), "ProStreet08Release.exe"))
        detected_game = GameType::PS;
    else if (detected_game == GameType::Unknown)
        detected_game = GameType::UC;

    if (detected_game != GameType::MW && detected_game != GameType::CB)
        Game::Init(detected_game);

    if (detected_game == GameType::Unknown)
    {
        MessageBoxA(NULL, Game::Error, Game::Name, MB_ICONERROR);
        return FALSE;
    }

    if (detected_game == GameType::MW)
    {
        if (!kUseIndependentRainFlow)
        {
            // Hook the rain tick callsite inside sub_6DE300 (0x006DF545).
            int callsite = EXE_ADDR(Game::RainTickAddr);
            CPatch::RedirectCall(callsite, HookedRainTickCallsite);

            unsigned char opcode = *reinterpret_cast<unsigned char*>(callsite);
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                          "[WeatherMod MainThread] rain callsite patched at 0x%08X opcode=0x%02X\n",
                          callsite, opcode);
            OutputDebugStringA(buf);

            // Hook Rain::Update/Render callsites to run in the native context.
            int updateCallsite = EXE_ADDR(Game::RainUpdateCallsiteAddr);
            CPatch::RedirectCall(updateCallsite, HookedRainUpdateCallsite);
            int renderCallsite = EXE_ADDR(Game::RainRenderCallsiteAddr);
            CPatch::RedirectCall(renderCallsite, HookedRainRenderCallsite);
            int renderCallsite2 = EXE_ADDR(Game::RainRenderCallsiteAddr2);
            CPatch::RedirectCall(renderCallsite2, HookedRainRenderCallsite);
            int renderCtxCallsite = EXE_ADDR(Game::RenderCtxCallsiteAddr);
            CPatch::RedirectCall(renderCtxCallsite, HookedRenderCtxCallsite);
            int renderCtxCallsite2 = EXE_ADDR(Game::RenderCtxCallsiteAddr2);
            CPatch::RedirectCall(renderCtxCallsite2, HookedRenderCtxCallsite2);
            {
                unsigned char opcode2 = *reinterpret_cast<unsigned char*>(renderCtxCallsite);
                char buf2[160];
                std::snprintf(buf2, sizeof(buf2),
                              "[WeatherMod MainThread] renderCtx callsite patched at 0x%08X opcode=0x%02X\n",
                              renderCtxCallsite, opcode2);
                OutputDebugStringA(buf2);
            }
            {
                unsigned char opcode3 = *reinterpret_cast<unsigned char*>(renderCtxCallsite2);
                char buf3[160];
                std::snprintf(buf3, sizeof(buf3),
                              "[WeatherMod MainThread] renderCtx2 callsite patched at 0x%08X opcode=0x%02X\n",
                              renderCtxCallsite2, opcode3);
                OutputDebugStringA(buf3);
            }
        }

        CreateThread(nullptr, 0, RainGuardWorker, nullptr, 0, nullptr);

        if (InitMinHook())
        {
            MH_CreateHook(reinterpret_cast<void*>(Game::eDisplayFrameAddr),
                          &HookedDisplayFrame,
                          reinterpret_cast<void**>(&g_originalDisplayFrame));
            MH_EnableHook(reinterpret_cast<void*>(Game::eDisplayFrameAddr));
        }
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
