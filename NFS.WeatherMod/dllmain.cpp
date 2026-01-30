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
#include "injector/injector.hpp"
#include <cstdio>
#include "minhook/include/MinHook.h"
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

    auto** devicePtr = reinterpret_cast<IDirect3DDevice9**>(MW::NFS_D3D9_DEVICE_ADDRESS);
    if (!core::IsReadable(devicePtr, sizeof(void*)))
        return nullptr;
    return *devicePtr;
}

static void __fastcall HookedRainTick(void* ecx, void* edx)
{
    (void)edx;
    static bool logged = false;
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

            auto* precipEnable = reinterpret_cast<int*>(MW::PRECIPITATION_ENABLE_ADDR);
            if (core::IsReadable(precipEnable, sizeof(int)))
                *precipEnable = 1;

            auto* precipPercent = reinterpret_cast<float*>(MW::PRECIPITATION_PERCENT_ADDR);
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
    if (!logged)
    {
        char buf[256];
        int precipEnable = 0;
        int precipRender = 0;
        float precipPercent = 0.0f;
        float rainPercent = 0.0f;
        int gameFlow = 0;
        int gameFlowStatus = 0;
        if (core::IsReadable(reinterpret_cast<void*>(MW::PRECIPITATION_ENABLE_ADDR), sizeof(int)))
            precipEnable = *reinterpret_cast<int*>(MW::PRECIPITATION_ENABLE_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(MW::PRECIPITATION_RENDER_ADDR), sizeof(int)))
            precipRender = *reinterpret_cast<int*>(MW::PRECIPITATION_RENDER_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(MW::PRECIPITATION_PERCENT_ADDR), sizeof(float)))
            precipPercent = *reinterpret_cast<float*>(MW::PRECIPITATION_PERCENT_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(MW::PRECIP_RAINPERCENT_ADDR), sizeof(float)))
            rainPercent = *reinterpret_cast<float*>(MW::PRECIP_RAINPERCENT_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(MW::GAMEFLOWMGR_ADDR), sizeof(int)))
            gameFlow = *reinterpret_cast<int*>(MW::GAMEFLOWMGR_ADDR);
        if (core::IsReadable(reinterpret_cast<void*>(MW::GAMEFLOWMGR_STATUS_ADDR), sizeof(int)))
            gameFlowStatus = *reinterpret_cast<int*>(MW::GAMEFLOWMGR_STATUS_ADDR);
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

// Forward declarations for per-frame logic
static void OnFrameUpdate();
static void HandleRainToggle();

static void __cdecl HookedDisplayFrame()
{
    // Get device from game global pointer each frame
    IDirect3DDevice9* device = GetGameDevice();
    if (device && !PrecipitationController::Get()->m_device)
        PrecipitationController::Get()->m_device = device;

    core::CurrentTime = timeGetTime();

    // Per-frame logic (initialization, toggle, update)
    OnFrameUpdate();
    HandleRainToggle();

    if (PrecipitationController::Get()->IsActive())
        PrecipitationController::Get()->Update();

    // Force native rain globals if 3D rain is enabled
    if (RainConfigController::precipitationConfig.enable3DRain ||
        RainConfigController::precipitationConfig.enable3DSplatters)
    {
        auto* rainEnable = reinterpret_cast<int*>(Game::RainEnablePtr);
        auto* particleEnable = reinterpret_cast<int*>(Game::ParticleSystemEnablePtr);
        if (core::IsReadable(rainEnable, sizeof(int)))
            *rainEnable = 1;
        if (core::IsReadable(particleEnable, sizeof(int)))
            *particleEnable = 1;

        auto* precipEnable = reinterpret_cast<int*>(MW::PRECIPITATION_ENABLE_ADDR);
        if (core::IsReadable(precipEnable, sizeof(int)))
            *precipEnable = 1;

        auto* precipPercent = reinterpret_cast<float*>(MW::PRECIPITATION_PERCENT_ADDR);
        if (core::IsReadable(precipPercent, sizeof(float)))
            *precipPercent = 1.0f;
    }

    if (g_originalDisplayFrame)
        g_originalDisplayFrame();
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
            PrecipitationController::Get()->enable();
            OutputDebugStringA("[RainToggle] Rain enabled\n");
            rainEnabled = true;
            lastKeyState = keyPressed;
            return;
        }

        bool canEnable = false;
        D3DXVECTOR3 cam = PrecipitationController::Get()->GetCameraPositionSafe();
        if (cam != D3DXVECTOR3(0, 0, 0))
            canEnable = true;

        if (canEnable)
        {
            PrecipitationController::Get()->enable();
            OutputDebugStringA("[RainToggle] Camera valid, rain enabled\n");
            rainEnabled = true;
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
        if (InitMinHook())
        {
            MH_CreateHook(
                reinterpret_cast<void*>(Game::CreateLookAtAddr),
                &HookedCreateLookAt,
                reinterpret_cast<void**>(&g_originalCreateLookAt));
            MH_EnableHook(reinterpret_cast<void*>(Game::CreateLookAtAddr));

            MH_CreateHook(
                reinterpret_cast<void*>(Game::BuildViewMatrixAddr),
                &HookedBuildView,
                reinterpret_cast<void**>(&g_originalBuildView));
            MH_EnableHook(reinterpret_cast<void*>(Game::BuildViewMatrixAddr));

            MH_CreateHook(
                reinterpret_cast<void*>(Game::BuildRenderViewAddr),
                &HookedBuildRenderView,
                reinterpret_cast<void**>(&g_originalBuildRenderView));
            MH_EnableHook(reinterpret_cast<void*>(Game::BuildRenderViewAddr));

            MH_CreateHook(
                reinterpret_cast<void*>(Game::BuildRenderMatrixAddr),
                &HookedBuildRenderMatrix,
                reinterpret_cast<void**>(&g_originalBuildRenderMatrix));
            MH_EnableHook(reinterpret_cast<void*>(Game::BuildRenderMatrixAddr));

            MH_CreateHook(
                reinterpret_cast<void*>(Game::eDisplayFrameAddr),
                &HookedDisplayFrame,
                reinterpret_cast<void**>(&g_originalDisplayFrame));
            MH_EnableHook(reinterpret_cast<void*>(Game::eDisplayFrameAddr));

            injector::MakeCALL(Game::RainTickAddr, HookedRainTick, true);

            MH_CreateHook(
                reinterpret_cast<void*>(Game::RainRender),
                &HookedRainRender,
                reinterpret_cast<void**>(&Game::g_originalRainRender));
            MH_EnableHook(reinterpret_cast<void*>(Game::RainRender));

            OutputDebugStringA("[WeatherMod MainThread] hooks installed (no D3D9 Present hook)\n");
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
