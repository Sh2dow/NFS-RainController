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
#include "PrecipitationConfigController.h"
#include "NFSMW_PreFEngHook.h"
#include "PrecipitationFlowMW/PrecipitationFlowMW.h"

#include "injector/injector.hpp"
#include <cstdio>
#include "minhook/include/MinHook.h"
#include "CPatch.h"
#include "GameAddresses.h"
// ==========================================================
// DLL ENTRY
// ==========================================================
GameType detected_game = GameType::Unknown;

static std::vector<std::unique_ptr<Feature>> g_features;
static bool triedInit = false;
static volatile LONG g_renderCustomPrecip = 0;
static bool g_endSceneHooked = false;
using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9* device);
static EndScene_t g_originalEndScene = nullptr;

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
using StuffSkyLayerBlend_t = void(__cdecl*)(void* view, float blend, int layer);
using StuffSkyLayer_t = void(__cdecl*)(void* view, int layer, float blend);
static StuffSkyLayer_t g_originalStuffSkyLayer = nullptr;
using ReplaceSkyTextures_t = void(__cdecl*)(int layer);
static ReplaceSkyTextures_t g_originalReplaceSkyTextures = nullptr;
static void __cdecl HookedReplaceSkyTexturesCallsite(int layer);
using AttachReplacementTextureTable_t = int(__thiscall*)(void* model, void* table, int a2, int a3);
static AttachReplacementTextureTable_t g_originalAttachReplacementTextureTable = nullptr;
static int __fastcall HookedAttachReplacementTextureTable(void* ecx, void*, void* table, int a2, int a3);
using SkyLayerCompute_t = void(__cdecl*)(void* view, int layer, float* out0, float* out1, float* out2, float* out3);
static SkyLayerCompute_t g_originalSkyLayerCompute = nullptr;
static void __cdecl HookedSkyLayerCompute(void* view, int layer, float* out0, float* out1, float* out2, float* out3);
static void __cdecl HookedSkyLayerComputeCallsite(void* view, int layer, float* out0, float* out1, float* out2,
                                                  float* out3);
using TimeOfDayUpdate_t = void(__cdecl*)(void* tod, float val);
static TimeOfDayUpdate_t g_originalTimeOfDayUpdate = nullptr;
static void __cdecl HookedTimeOfDayUpdate(void* tod, float val);

static void __fastcall HookedRainUpdateCallsite(void* ecx, void*);
static void __fastcall HookedRainRenderCallsite(void* ecx, void*);
static void HookedRenderCtxCallsite();
static void AfterRenderCtxCallsite();
static constexpr bool kUseIndependentRainFlow = true;
static constexpr bool kUseIndependentSkyFlow = true;
static volatile bool g_rainRequested = false;
static void __cdecl HookedStuffSkyLayerBlendCallsite(void* view, float blend, int layer);
static void __cdecl HookedStuffSkyLayer(void* view, int layer, float blend);
static void __cdecl HookedStuffSkyLayerCallsite(void* view, int layer, float blend);
static void __cdecl HookedReplaceSkyTextures(int layer);

static void ForceDryStateIfDisabled()
{
    if (!PrecipitationController::Get()->IsActive())
    {
        float smoothed = PrecipitationFlowMW::GetSmoothedRain();
        if (smoothed > 0.01f)
            return;
        if (Game::PRECIP_RAINOVERRIDE_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINOVERRIDE_ADDR) = 0.0f;
        if (Game::FOG_CTRLOVERRIDE_ADDR)
            *reinterpret_cast<int*>(Game::FOG_CTRLOVERRIDE_ADDR) = 0;
        if (Game::RoadReflectionStateAddr)
            *reinterpret_cast<int*>(Game::RoadReflectionStateAddr) = 2;
    }
}

static void __cdecl HookedDisplayFrame()
{
    if (g_originalDisplayFrame)
        g_originalDisplayFrame();
    ForceDryStateIfDisabled();
}

static HRESULT __stdcall HookedEndScene(IDirect3DDevice9* device)
{
    if (InterlockedCompareExchange(&g_renderCustomPrecip, 0, 0) != 0)
    {
        auto* controller = PrecipitationController::Get();
        controller->m_device = device;
        controller->Update();
    }
    return g_originalEndScene ? g_originalEndScene(device) : D3D_OK;
}

static void RunNativeRainFlow(void* rain)
{
    if (kUseIndependentRainFlow)
        return;
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
    float rainPct = PrecipitationConfigController::precipitationConfig.rainIntensity;
    if (rainPct < 0.0f)
        rainPct = 0.0f;
    float fogPct = PrecipitationConfigController::precipitationConfig.fogIntensity;
    if (fogPct < 0.0f)
        fogPct = 0.0f;
    if (!loggedOnce)
    {
        char dbg[256];
        std::snprintf(dbg, sizeof(dbg),
                      "[WeatherMod] cfg rain=%.3f fog=%.3f enable3DRain=%d enable3DSplatters=%d\n",
                      rainPct, fogPct,
                      PrecipitationConfigController::precipitationConfig.enable3DRain ? 1 : 0,
                      PrecipitationConfigController::precipitationConfig.enable3DSplatters ? 1 : 0);
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

    // Native tuning globals are handled by preset application; avoid clobbering here.

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

                float rainPct = PrecipitationConfigController::precipitationConfig.rainIntensity;
                if (rainPct < 0.0f)
                    rainPct = 0.0f;
                float fogPct = PrecipitationConfigController::precipitationConfig.fogIntensity;
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
    static float lastEndPct = -1.0f;
    float endPct = *reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR);
    float smoothed = PrecipitationFlowMW::GetSmoothedRain();
    float smoothedFog = PrecipitationFlowMW::GetSmoothedFog();
    if (kUseIndependentSkyFlow && fabsf(endPct - smoothed) > 0.01f)
    {
        *reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR) = smoothed;
        *reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR) = smoothed;
        *reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR) = smoothedFog;
        if (Game::PRECIP_RAINOVERRIDE_ADDR)
            *reinterpret_cast<float*>(Game::PRECIP_RAINOVERRIDE_ADDR) = smoothed;
        endPct = smoothed;
    }
    if (fabsf(endPct - lastEndPct) > 0.01f)
    {
        char dbg[128];
        std::snprintf(dbg, sizeof(dbg), "[WeatherMod] endframe pct=%.3f\n", endPct);
        OutputDebugStringA(dbg);
        lastEndPct = endPct;
    }
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
    if (PrecipitationConfigController::precipitationConfig.enable3DRain ||
        PrecipitationConfigController::precipitationConfig.enable3DSplatters)
    {
        return;
    }
    if (detected_game == GameType::MW)
    {
        if (PrecipitationConfigController::precipitationConfig.enable3DRain ||
            PrecipitationConfigController::precipitationConfig.enable3DSplatters)
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
    if (PrecipitationConfigController::precipitationConfig.enable3DRain ||
        PrecipitationConfigController::precipitationConfig.enable3DSplatters)
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

static void __cdecl HookedStuffSkyLayerBlendCallsite(void* view, float blend, int layer)
{
    if (kUseIndependentRainFlow)
    {
        float pct = PrecipitationFlowMW::GetSmoothedRain();
        if (pct < 0.0f)
            pct = 0.0f;
        if (pct > 1.0f)
            pct = 1.0f;
        blend = pct;
    }

    auto fn = reinterpret_cast<StuffSkyLayerBlend_t>(Game::StuffSkyLayerBlendAddr);
    if (fn)
        fn(view, blend, layer);
}

static void __cdecl HookedStuffSkyLayer(void* view, int layer, float blend)
{
    static float lastLogged = -1.0f;
    if (kUseIndependentRainFlow)
    {
        float pct = PrecipitationFlowMW::GetSmoothedRain();
        if (pct < 0.0f)
            pct = 0.0f;
        if (pct > 1.0f)
            pct = 1.0f;
        blend = pct;
    }

    if (fabsf(blend - lastLogged) > 0.05f)
    {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "[WeatherMod] StuffSkyLayer hit layer=%d blend=%.3f\n",
                      layer, blend);
        OutputDebugStringA(buf);
        lastLogged = blend;
    }

    if (g_originalStuffSkyLayer)
        g_originalStuffSkyLayer(view, layer, blend);
}

static void __cdecl HookedStuffSkyLayerCallsite(void* view, int layer, float blend)
{
    if (kUseIndependentRainFlow)
        blend = PrecipitationFlowMW::GetSmoothedRain();
    auto fn = reinterpret_cast<StuffSkyLayer_t>(Game::StuffSkyLayerAddr);
    if (fn)
        fn(view, layer, blend);
}

static void __cdecl HookedReplaceSkyTextures(int layer)
{
    static bool logged = false;
    if (kUseIndependentRainFlow)
    {
        static bool lastRainy = false;
        float pct = PrecipitationFlowMW::GetSmoothedRain();
        bool targetRainy = pct >= 0.5f;

        // Delay texture swap until transition is mostly complete.
        if (targetRainy != lastRainy)
        {
            if ((targetRainy && pct < 0.95f) || (!targetRainy && pct > 0.05f))
                return;
            lastRainy = targetRainy;
        }
    }

    if (!logged)
    {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
                      "[WeatherMod] ReplaceSkyTextures hit layer=%d pct=%.3f\n",
                      layer, PrecipitationFlowMW::GetSmoothedRain());
        OutputDebugStringA(buf);
        logged = true;
    }

    if (g_originalReplaceSkyTextures)
        g_originalReplaceSkyTextures(layer);
}

static void __cdecl HookedReplaceSkyTexturesCallsite(int layer)
{
    HookedReplaceSkyTextures(layer);
}

static int __fastcall HookedAttachReplacementTextureTable(void* ecx, void*, void* table, int a2, int a3)
{
    if (kUseIndependentRainFlow)
    {
        bool isSkyTable = false;
        if (core::IsReadable(table, 0x80))
        {
            // Hash first 0x80 bytes to fingerprint tables.
            uint32_t hash = 2166136261u;
            auto* bytes = reinterpret_cast<unsigned char*>(table);
            for (size_t i = 0; i < 0x80; ++i)
                hash = (hash ^ bytes[i]) * 16777619u;
            isSkyTable = (hash == 0x72246BF1u);
        }

        if (!isSkyTable)
        {
            if (g_originalAttachReplacementTextureTable)
                return g_originalAttachReplacementTextureTable(ecx, table, a2, a3);
            return 0;
        }

        float pct = PrecipitationFlowMW::GetSmoothedRain();
        bool targetRainy = pct >= 0.5f;
        static bool lastRainy = false;
        if (targetRainy != lastRainy)
        {
            if ((targetRainy && pct < 0.95f) || (!targetRainy && pct > 0.05f))
                return 0; // skip swap during transition
            lastRainy = targetRainy;
        }
    }

    if (g_originalAttachReplacementTextureTable)
        return g_originalAttachReplacementTextureTable(ecx, table, a2, a3);
    return 0;
}

static void __cdecl HookedSkyLayerCompute(void* view, int layer, float* out0, float* out1, float* out2, float* out3)
{
    if (!g_originalSkyLayerCompute)
        return;

    if (!kUseIndependentRainFlow)
    {
        g_originalSkyLayerCompute(view, layer, out0, out1, out2, out3);
        return;
    }

    float savedAccum = 0.0f;
    float savedSky = 0.0f;
    if (Game::WeatherBlendAccumAddr)
        savedAccum = *reinterpret_cast<float*>(Game::WeatherBlendAccumAddr);
    if (Game::WeatherSkyBlendVarAddr)
        savedSky = *reinterpret_cast<float*>(Game::WeatherSkyBlendVarAddr);

    float clear0 = 0.0f, clear1 = 0.0f, clear2 = 0.0f, clear3 = 0.0f;
    float rain0 = 0.0f, rain1 = 0.0f, rain2 = 0.0f, rain3 = 0.0f;

    if (Game::WeatherBlendAccumAddr)
        *reinterpret_cast<float*>(Game::WeatherBlendAccumAddr) = 0.0f;
    if (Game::WeatherSkyBlendVarAddr)
        *reinterpret_cast<float*>(Game::WeatherSkyBlendVarAddr) = 0.0f;
    g_originalSkyLayerCompute(view, layer, &clear0, &clear1, &clear2, &clear3);

    if (Game::WeatherBlendAccumAddr)
        *reinterpret_cast<float*>(Game::WeatherBlendAccumAddr) = 1.0f;
    if (Game::WeatherSkyBlendVarAddr)
        *reinterpret_cast<float*>(Game::WeatherSkyBlendVarAddr) = 1.0f;
    g_originalSkyLayerCompute(view, layer, &rain0, &rain1, &rain2, &rain3);

    if (Game::WeatherBlendAccumAddr)
        *reinterpret_cast<float*>(Game::WeatherBlendAccumAddr) = savedAccum;
    if (Game::WeatherSkyBlendVarAddr)
        *reinterpret_cast<float*>(Game::WeatherSkyBlendVarAddr) = savedSky;

    float t = PrecipitationFlowMW::GetSmoothedRain();
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    if (out0) *out0 = clear0 + (rain0 - clear0) * t;
    if (out1) *out1 = clear1 + (rain1 - clear1) * t;
    if (out2) *out2 = clear2 + (rain2 - clear2) * t;
    if (out3) *out3 = clear3 + (rain3 - clear3) * t;

    static bool logged = false;
    if (!logged)
    {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "[WeatherMod] SkyLayerCompute layer=%d clear=%.3f/%.3f/%.3f/%.3f rain=%.3f/%.3f/%.3f/%.3f t=%.3f\n",
                      layer, clear0, clear1, clear2, clear3, rain0, rain1, rain2, rain3, t);
        OutputDebugStringA(buf);
        logged = true;
    }
}

static void __cdecl HookedTimeOfDayUpdate(void* tod, float val)
{
    if (kUseIndependentRainFlow && kUseIndependentSkyFlow)
        val = PrecipitationFlowMW::GetSmoothedRain();
    if (g_originalTimeOfDayUpdate)
        g_originalTimeOfDayUpdate(tod, val);
}

static void __cdecl HookedSkyLayerComputeCallsite(void* view, int layer, float* out0, float* out1, float* out2,
                                                  float* out3)
{
    HookedSkyLayerCompute(view, layer, out0, out1, out2, out3);
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
    PrecipitationConfigController::LoadOnStartup();
    PrecipitationConfigController::Load();

    for (const auto& feature : g_features)
    {
        const char* name = feature->name();
        OutputDebugStringA(name);
        OutputDebugStringA(" initialized\n");

        if (strcmp(name, "PrecipitationController") == 0)
        {
            auto* controller = PrecipitationController::Get();

            if (PrecipitationConfigController::precipitationConfig.enableOnStartup)
            {
                controller->DebugEVIEWListPtr();

                if (!controller->m_rainTex && !PrecipitationConfigController::precipitationConfig.use_raindrop_dds)
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

    bool keyPressed = (GetAsyncKeyState(PrecipitationConfigController::toggleKey) & 0x8000) != 0;

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
        if (!PrecipitationController::Get()->IsActive())
        {
            PrecipitationController::Get()->enable();
            OutputDebugStringA("[RainToggle] Rain enabled\n");
        }
        rainEnabled = PrecipitationController::Get()->IsActive();
        lastKeyState = keyPressed;
        return;

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

    if (!shouldEnable && !rainEnabled && PrecipitationConfigController::precipitationConfig.enableOnStartup)
    {
        shouldEnable = true;
    }

    g_rainRequested = shouldEnable;
    lastKeyState = keyPressed;
}

static void ApplyNativePresetGlobalsMW()
{
    const auto& p = PrecipitationConfigController::precipitationConfig.nativePreset;
    static bool loggedPreset = false;
    if (!loggedPreset)
    {
        char buf[256];
        sprintf_s(buf,
                  "[WeatherMod] Preset=%s applyPreset=%d (RainGuardWorker)\n",
                  PrecipitationConfigController::precipitationConfig.presetName.c_str(),
                  PrecipitationConfigController::precipitationConfig.applyPresetGlobals ? 1 : 0);
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
    if (Game::PRECIP_RAINZCONSTANT_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINZCONSTANT_ADDR),
                                                            sizeof(float)))
        beforeGrav = *reinterpret_cast<float*>(Game::PRECIP_RAINZCONSTANT_ADDR);
    if (Game::PRECIP_BASEDAMPNESS_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_BASEDAMPNESS_ADDR),
                                                           sizeof(float)))
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
    if (Game::RoadReflectionEnablePtr)
        *reinterpret_cast<float*>(Game::RoadReflectionEnablePtr) = p.roadReflectionEnable;

    static int readbackCountdown = 3;
    if (readbackCountdown > 0)
    {
        --readbackCountdown;
        float afterCross = beforeCross;
        float afterFall = beforeFall;
        float afterGrav = beforeGrav;
        float afterDamp = beforeDamp;
        if (Game::PRECIP_RAINY_ADDR &&
            core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINY_ADDR), sizeof(float)))
            afterCross = *reinterpret_cast<float*>(Game::PRECIP_RAINY_ADDR);
        if (Game::PRECIP_RAINZ_ADDR &&
            core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_RAINZ_ADDR), sizeof(float)))
            afterFall = *reinterpret_cast<float*>(Game::PRECIP_RAINZ_ADDR);
        if (Game::PRECIP_RAINZCONSTANT_ADDR && core::IsReadable(
            reinterpret_cast<void*>(Game::PRECIP_RAINZCONSTANT_ADDR), sizeof(float)))
            afterGrav = *reinterpret_cast<float*>(Game::PRECIP_RAINZCONSTANT_ADDR);
        if (Game::PRECIP_BASEDAMPNESS_ADDR && core::IsReadable(reinterpret_cast<void*>(Game::PRECIP_BASEDAMPNESS_ADDR),
                                                               sizeof(float)))
            afterDamp = *reinterpret_cast<float*>(Game::PRECIP_BASEDAMPNESS_ADDR);
        char buf[256];
        sprintf_s(buf,
                  "[WeatherMod] preset readback cross=%.3f->%.3f fall=%.3f->%.3f grav=%.3f->%.3f damp=%.3f->%.3f\n",
                  beforeCross, afterCross, beforeFall, afterFall, beforeGrav, afterGrav, beforeDamp, afterDamp);
        OutputDebugStringA(buf);
    }
}

// Helper: keep engine rain flags set so the native callsite runs.
static DWORD WINAPI RainGuardWorker(void*)
{
    int decayFrames = 0;
    while (true)
    {
        OnFrameUpdate();
        HandleRainToggle();

        if (g_rainRequested && !PrecipitationController::Get()->IsActive())
            PrecipitationController::Get()->enable();

        const bool enabled = PrecipitationController::Get()->IsActive();
        if (enabled && PrecipitationConfigController::precipitationConfig.applyPresetRendering)
            PrecipitationController::Get()->enable();
        float targetRain = enabled ? PrecipitationConfigController::precipitationConfig.rainIntensity : 0.0f;
        float targetFog = enabled ? PrecipitationConfigController::precipitationConfig.fogIntensity : 0.0f;
        PrecipitationFlowMW::SetTargets(targetRain, targetFog);
        if (kUseIndependentRainFlow)
            PrecipitationFlowMW::Tick();

        if (enabled && PrecipitationConfigController::precipitationConfig.applyPresetRendering)
            InterlockedExchange(&g_renderCustomPrecip, 1);
        else
            InterlockedExchange(&g_renderCustomPrecip, 0);

        float smoothed = kUseIndependentRainFlow ? PrecipitationFlowMW::GetSmoothedRain() : 0.0f;
        const bool keepAlive = smoothed > 0.01f;
        const bool isRainPreset = (PrecipitationConfigController::precipitationConfig.presetName == "Rain");
        if (enabled || keepAlive)
        {
            // *reinterpret_cast<int*>(Game::PRECIPITATION_DEBUG_ADDR) = 1;
            // Preset globals are applied from PrecipitationController::Update (EndScene path).
            // Avoid duplicate writes from worker thread.

            // decayFrames = 0;
            // if (PrecipitationConfigController::precipitationConfig.applyPresetRendering)
            // {
            //     // Custom renderer path (e.g., snow) — disable native precipitation.
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR) = 0;
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR) = 0;
            //     *reinterpret_cast<int*>(Game::RainEnablePtr) = 0;
            //     *reinterpret_cast<int*>(Game::ParticleSystemEnablePtr) = 0;
            // }
            // else
            // {
            //     // Force rain/particle globals so the engine doesn't skip the native block.
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR) = 1;
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR) = 1;
            //     *reinterpret_cast<int*>(Game::RainEnablePtr) = 1;
            //     *reinterpret_cast<int*>(Game::ParticleSystemEnablePtr) = 1;
            // }

            if (isRainPreset)
            {
                injector::WriteMemory<float>(Game::PRECIP_BASEDAMPNESS_ADDR, true);
                injector::WriteMemory<uint8_t>(Game::RoadReflectionFix, 0xEB, true);
            }
            // else
            // {
            //     injector::WriteMemory<float>(Game::PRECIP_BASEDAMPNESS_ADDR, false);
            //     injector::WriteMemory<uint8_t>(Game::RoadReflectionFix, 0x74, true);
            // }
            // *reinterpret_cast<uint8_t*>(Game::PRECIPITATION_DEBUG_ADDR) = 0xEB;
            *reinterpret_cast<int*>(Game::PRECIPITATION_DEBUG_ADDR) = 1;
        }
        else
        {
            injector::WriteMemory<float>(Game::PRECIP_BASEDAMPNESS_ADDR, false);
            // Restore original opcode for `jz short loc_7582A2` at 0x758293.
            injector::WriteMemory<uint8_t>(Game::RoadReflectionFix, 0x74, true);
            *reinterpret_cast<int*>(Game::PRECIPITATION_DEBUG_ADDR) = 0;

            // if (PrecipitationConfigController::precipitationConfig.applyPresetRendering)
            // {
            //     if (Game::PRECIP_RAINOVERRIDE_ADDR)
            //         *reinterpret_cast<float*>(Game::PRECIP_RAINOVERRIDE_ADDR) = 0.0f;
            //     if (Game::PRECIP_FOGPERCENT_ADDR)
            //         *reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR) = 0.0f;
            //     if (Game::FOG_CTRLOVERRIDE_ADDR)
            //         *reinterpret_cast<int*>(Game::FOG_CTRLOVERRIDE_ADDR) = 0;
            // }
            // Let a few frames pass at zero to avoid a hard snap on disable.
            // if (++decayFrames >= 30)
            // {
            //     // Finalize shutdown after ramp reaches ~0.
            //     if (kUseIndependentRainFlow)
            //         PrecipitationFlowMW::Disable();
            //     if (Game::PRECIP_RAINOVERRIDE_ADDR)
            //         *reinterpret_cast<float*>(Game::PRECIP_RAINOVERRIDE_ADDR) = 0.0f;
            //     if (Game::FOG_CTRLOVERRIDE_ADDR)
            //         *reinterpret_cast<int*>(Game::FOG_CTRLOVERRIDE_ADDR) = 0;
            //     if (Game::RoadReflectionStateAddr)
            //         *reinterpret_cast<int*>(Game::RoadReflectionStateAddr) = 2;
            //     if (core::IsReadable(reinterpret_cast<void*>(Game::RainInstancePtr), sizeof(void*)))
            //     {
            //         uint8_t* rain = *reinterpret_cast<uint8_t**>(Game::RainInstancePtr);
            //         if (core::IsReadable(rain, 0x300))
            //             *reinterpret_cast<int*>(rain + 0x280) = 0;
            //     }
            //     *reinterpret_cast<float*>(Game::PRECIP_RAINPERCENT_ADDR) = 0.0f;
            //     *reinterpret_cast<float*>(Game::PRECIP_FOGPERCENT_ADDR) = 0.0f;
            //     *reinterpret_cast<float*>(Game::PRECIPITATION_PERCENT_ADDR) = 0.0f;
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR) = 0;
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR) = 0;
            //     *reinterpret_cast<int*>(Game::RainEnablePtr) = 0;
            //     *reinterpret_cast<int*>(Game::ParticleSystemEnablePtr) = 0;
            // }
            // else
            // {
            //     // Keep flags alive while the last frames settle.
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_ENABLE_ADDR) = 1;
            //     *reinterpret_cast<int*>(Game::PRECIPITATION_RENDER_ADDR) = 1;
            //     *reinterpret_cast<int*>(Game::RainEnablePtr) = 1;
            //     *reinterpret_cast<int*>(Game::ParticleSystemEnablePtr) = 1;
            // }
        }

        // Ensure particle context pointer is valid for sub_6DE300 path.
        // void** particleCtx = reinterpret_cast<void**>(Game::particleCtxAddr);
        // void** renderCtx = reinterpret_cast<void**>(Game::renderCtxAddr);
        // if (core::IsReadable(particleCtx, sizeof(void*)) && *particleCtx)
        // {
        //     if (core::IsReadable(renderCtx, sizeof(void*)) && !*renderCtx)
        //         *renderCtx = *particleCtx;
        // }

        if (!g_endSceneHooked && Game::NFS_D3D9_DEVICE_ADDRESS)
        {
            auto** devPtr = reinterpret_cast<IDirect3DDevice9**>(Game::NFS_D3D9_DEVICE_ADDRESS);
            if (core::IsReadable(devPtr, sizeof(void*)) && *devPtr)
            {
                IDirect3DDevice9* dev = *devPtr;
                void** vtbl = *reinterpret_cast<void***>(dev);
                if (vtbl)
                {
                    void* target = vtbl[42]; // EndScene
                    if (MH_CreateHook(target, &HookedEndScene, reinterpret_cast<void**>(&g_originalEndScene)) == MH_OK
                        &&
                        MH_EnableHook(target) == MH_OK)
                    {
                        g_endSceneHooked = true;
                        OutputDebugStringA("[WeatherMod] EndScene hook installed\n");
                    }
                    else
                    {
                        OutputDebugStringA("[WeatherMod] EndScene hook failed\n");
                    }
                }
            }
        }
    }
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
        PrecipitationFlowMW::SetUseGameSkyFlow(kUseIndependentSkyFlow);
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

        MH_STATUS status = MH_Initialize();
        if (status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED)
        {
            if (Game::eDisplayFrameAddr)
            {
                auto stDisplay = MH_CreateHook(reinterpret_cast<void*>(Game::eDisplayFrameAddr),
                                               &HookedDisplayFrame,
                                               reinterpret_cast<void**>(&g_originalDisplayFrame));
                auto enDisplay = MH_EnableHook(reinterpret_cast<void*>(Game::eDisplayFrameAddr));
                if (stDisplay != MH_OK || enDisplay != MH_OK)
                    OutputDebugStringA("[WeatherMod] DisplayFrame hook failed\n");
                else
                    OutputDebugStringA("[WeatherMod] DisplayFrame hook installed\n");
            }

            // if (Game::StuffSkyLayerAddr)
            // {
            //     auto stSky = MH_CreateHook(reinterpret_cast<void*>(Game::StuffSkyLayerAddr),
            //                                &HookedStuffSkyLayer,
            //                                reinterpret_cast<void**>(&g_originalStuffSkyLayer));
            //     auto enSky = MH_EnableHook(reinterpret_cast<void*>(Game::StuffSkyLayerAddr));
            //     if (stSky != MH_OK || enSky != MH_OK)
            //         OutputDebugStringA("[WeatherMod] StuffSkyLayer hook failed\n");
            // }
            //
            // if (Game::ReplaceSkyTexturesAddr)
            // {
            //     auto stSkyTex = MH_CreateHook(reinterpret_cast<void*>(Game::ReplaceSkyTexturesAddr),
            //                                   &HookedReplaceSkyTextures,
            //                                   reinterpret_cast<void**>(&g_originalReplaceSkyTextures));
            //     auto enSkyTex = MH_EnableHook(reinterpret_cast<void*>(Game::ReplaceSkyTexturesAddr));
            //     if (stSkyTex != MH_OK || enSkyTex != MH_OK)
            //         OutputDebugStringA("[WeatherMod] ReplaceSkyTextures hook failed\n");
            //     else
            //         OutputDebugStringA("[WeatherMod] ReplaceSkyTextures hook installed\n");
            // }
            //
            // if (Game::AttachReplacementTextureTableAddr)
            // {
            //     auto stAttach = MH_CreateHook(reinterpret_cast<void*>(Game::AttachReplacementTextureTableAddr),
            //                                   &HookedAttachReplacementTextureTable,
            //                                   reinterpret_cast<void**>(&g_originalAttachReplacementTextureTable));
            //     auto enAttach = MH_EnableHook(reinterpret_cast<void*>(Game::AttachReplacementTextureTableAddr));
            //     if (stAttach != MH_OK || enAttach != MH_OK)
            //         OutputDebugStringA("[WeatherMod] AttachReplacementTextureTable hook failed\n");
            //     else
            //         OutputDebugStringA("[WeatherMod] AttachReplacementTextureTable hook installed\n");
            // }
            //
            // if (Game::SkyLayerComputeAddr)
            // {
            //     auto stSkyComp = MH_CreateHook(reinterpret_cast<void*>(Game::SkyLayerComputeAddr),
            //                                    &HookedSkyLayerCompute,
            //                                    reinterpret_cast<void**>(&g_originalSkyLayerCompute));
            //     auto enSkyComp = MH_EnableHook(reinterpret_cast<void*>(Game::SkyLayerComputeAddr));
            //     if (stSkyComp != MH_OK || enSkyComp != MH_OK)
            //         OutputDebugStringA("[WeatherMod] SkyLayerCompute hook failed\n");
            //     else
            //         OutputDebugStringA("[WeatherMod] SkyLayerCompute hook installed\n");
            // }
            //
            // if (Game::TimeOfDayUpdateAddr)
            // {
            //     auto stTOD = MH_CreateHook(reinterpret_cast<void*>(Game::TimeOfDayUpdateAddr),
            //                                &HookedTimeOfDayUpdate,
            //                                reinterpret_cast<void**>(&g_originalTimeOfDayUpdate));
            //     auto enTOD = MH_EnableHook(reinterpret_cast<void*>(Game::TimeOfDayUpdateAddr));
            //     if (stTOD != MH_OK || enTOD != MH_OK)
            //         OutputDebugStringA("[WeatherMod] TimeOfDayUpdate hook failed\n");
            //     else
            //         OutputDebugStringA("[WeatherMod] TimeOfDayUpdate hook installed\n");
            // }

            // FX_Weather hook disabled (crash observed).
        }
        else
        {
            OutputDebugStringA("[WeatherMod] MinHook init failed for DisplayFrame\n");
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
