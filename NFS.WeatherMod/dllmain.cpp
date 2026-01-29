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
#include "WeatherGameAddresses.h"
#include "NFSMW_PreFEngHook.h"
#include "injector/injector.hpp"
#include <cstdio>
#include "minhook/include/MinHook.h"
// ==========================================================
// DLL ENTRY
// ==========================================================
GameType detected_game = GameType::Unknown;

using namespace ngg::common;

static std::vector<std::unique_ptr<ngg::common::Feature>> g_features;
static bool g_hookTransferred = false;
static bool triedInit = false;

typedef HRESULT(APIENTRY* PresentFn)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT(APIENTRY* CreateDeviceFn)(
    IDirect3D9*,
    UINT,
    D3DDEVTYPE,
    HWND,
    DWORD,
    D3DPRESENT_PARAMETERS*,
    IDirect3DDevice9**);

static PresentFn g_originalPresent = nullptr;
static PresentFn g_originalDummyPresent = nullptr;
static CreateDeviceFn g_originalCreateDevice = nullptr;
typedef HRESULT(APIENTRY* SetTransformFn)(IDirect3DDevice9*, D3DTRANSFORMSTATETYPE, const D3DMATRIX*);
static SetTransformFn g_originalSetTransform = nullptr;

using CreateLookAt_t = void(__cdecl*)(Mat4* mat, Vec3* eye, Vec3* center, Vec3* up);
static CreateLookAt_t g_originalCreateLookAt = nullptr;
using BuildView_t = int(__fastcall*)(void* viewObj, void* edx, int a0, float a4, float a8, int aC, char a10);
static BuildView_t g_originalBuildView = nullptr;
using BuildRenderView_t = int(__cdecl*)(void* a0, int a4, int a8);
static BuildRenderView_t g_originalBuildRenderView = nullptr;
using BuildRenderMatrix_t = void(__cdecl*)(void* outMat, void* inMat);
static BuildRenderMatrix_t g_originalBuildRenderMatrix = nullptr;
using DisplayFrameMW_t = void(__cdecl*)();
static DisplayFrameMW_t g_originalDisplayFrameMW = nullptr;
using RainTick_t = void(__thiscall*)(void*);
static RainTick_t g_originalRainTickMW = reinterpret_cast<RainTick_t>(WeatherGameAddresses::RainTick_MW);
using RainRender_t = void(__thiscall*)(void*);
static RainRender_t g_originalRainRenderMW = reinterpret_cast<RainRender_t>(WeatherGameAddresses::RainRender_MW);

static void __fastcall HookedRainTickMW(void* ecx, void* edx)
{
    (void)edx;
    static bool logged = false;
    if (detected_game == GameType::MW)
    {
        if (RainConfigController::precipitationConfig.enable3DRain ||
            RainConfigController::precipitationConfig.enable3DSplatters)
        {
            auto* rainEnable = reinterpret_cast<int*>(WeatherGameAddresses::RainEnablePtr_MW);
            auto* particleEnable = reinterpret_cast<int*>(WeatherGameAddresses::ParticleSystemEnablePtr_MW);
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
        OutputDebugStringA("[RainDebug MW] HookedRainTickMW called\n");
        logged = true;
    }
    // Avoid running tick when render context is not set (dword_982C80 == 0).
    auto* renderCtx = reinterpret_cast<void*>(0x00982C80);
    if (!core::IsReadable(renderCtx, sizeof(void*)) || !*reinterpret_cast<void**>(renderCtx))
        return;
    if (g_originalRainTickMW)
        g_originalRainTickMW(ecx);
}

static void __fastcall HookedRainRenderMW(void* ecx, void* edx)
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
                      "[RainDebug MW] HookedRainRenderMW called precipEnable=%d precipRender=%d precipPct=%.2f rainPct=%.2f gameFlow=%d gameFlowStatus=%d\n",
                      precipEnable, precipRender, precipPercent, rainPercent, gameFlow, gameFlowStatus);
        OutputDebugStringA(buf);

        if (core::IsReadable(ecx, 0x290))
        {
            void* p284 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ecx) + 0x284);
            void* p288 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ecx) + 0x288);
            std::snprintf(buf, sizeof(buf),
                          "[RainDebug MW] Rain instance ptr=0x%p p284=0x%p p288=0x%p\n",
                          ecx, p284, p288);
            OutputDebugStringA(buf);

            if (!p284 && p288 && core::IsReadable(p288, 0x70))
            {
                void* viewPlat = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(p288) + 0x44);
                if (viewPlat)
                {
                    *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ecx) + 0x284) = viewPlat;
                    OutputDebugStringA("[RainDebug MW] Patched Rain::mPtr284 = *(eView+0x44) (viewPlat)\n");
                }
            }
        }
        logged = true;
    }
    // Avoid crashing when render context is not set (dword_982C80 == 0).
    auto* renderCtx = reinterpret_cast<void*>(0x00982C80);
    void* ctxVal = nullptr;
    if (core::IsReadable(renderCtx, sizeof(void*)))
        ctxVal = *reinterpret_cast<void**>(renderCtx);

    // If render context is missing, try to seed it from particle system context.
    if (!ctxVal)
    {
        auto* particleCtx = reinterpret_cast<void*>(0x0093DEC0);
        if (core::IsReadable(particleCtx, sizeof(void*)) && *reinterpret_cast<void**>(particleCtx))
        {
            *reinterpret_cast<void**>(renderCtx) = *reinterpret_cast<void**>(particleCtx);
            ctxVal = *reinterpret_cast<void**>(renderCtx);
        }
    }

    if (ctxVal && g_originalRainRenderMW)
        g_originalRainRenderMW(ecx);
}

static void __cdecl HookedDisplayFrameMW()
{
    if (detected_game == GameType::MW)
    {
        if (RainConfigController::precipitationConfig.enable3DRain ||
            RainConfigController::precipitationConfig.enable3DSplatters)
        {
            auto* rainEnable = reinterpret_cast<int*>(WeatherGameAddresses::RainEnablePtr_MW);
            auto* particleEnable = reinterpret_cast<int*>(WeatherGameAddresses::ParticleSystemEnablePtr_MW);
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
    }
    if (g_originalDisplayFrameMW)
        g_originalDisplayFrameMW();
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
        PrecipitationController::UpdateMWViewMatrix(view);
        if (!logged)
        {
            OutputDebugStringA("[RainDebug MW] HookedCreateLookAt fired\n");
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
        PrecipitationController::UpdateMWViewMatrix(view);
    }
    return result;
}

static HRESULT APIENTRY HookedSetTransform(IDirect3DDevice9* device, D3DTRANSFORMSTATETYPE state,
                                           const D3DMATRIX* mat)
{
    static bool loggedView = false;
    static bool loggedProj = false;
    static bool loggedViewNonId = false;
    static bool loggedProjNonId = false;
    static bool loggedViewId = false;
    static bool loggedProjId = false;
    auto isIdentity = [](const D3DMATRIX* m) -> bool
    {
        if (!m) return true;
        return m->m[0][0] == 1.0f && m->m[1][1] == 1.0f && m->m[2][2] == 1.0f && m->m[3][3] == 1.0f &&
            m->m[0][1] == 0.0f && m->m[0][2] == 0.0f && m->m[0][3] == 0.0f &&
            m->m[1][0] == 0.0f && m->m[1][2] == 0.0f && m->m[1][3] == 0.0f &&
            m->m[2][0] == 0.0f && m->m[2][1] == 0.0f && m->m[2][3] == 0.0f &&
            m->m[3][0] == 0.0f && m->m[3][1] == 0.0f && m->m[3][2] == 0.0f;
    };
    if (detected_game == GameType::MW)
    {
        if (!isIdentity(mat))
        {
            PrecipitationController::UpdateD3DTransform(state, mat);
            if (state == D3DTS_VIEW && !loggedViewNonId)
            {
                OutputDebugStringA("[RainDebug MW] HookedSetTransform VIEW non-identity captured\n");
                loggedViewNonId = true;
            }
            else if (state == D3DTS_PROJECTION && !loggedProjNonId)
            {
                OutputDebugStringA("[RainDebug MW] HookedSetTransform PROJ non-identity captured\n");
                loggedProjNonId = true;
            }
        }
        else
        {
            if (state == D3DTS_VIEW && !loggedViewId)
            {
                OutputDebugStringA("[RainDebug MW] HookedSetTransform VIEW identity ignored\n");
                loggedViewId = true;
            }
            else if (state == D3DTS_PROJECTION && !loggedProjId)
            {
                OutputDebugStringA("[RainDebug MW] HookedSetTransform PROJ identity ignored\n");
                loggedProjId = true;
            }
        }
    }
    if (state == D3DTS_VIEW && !loggedView)
    {
        OutputDebugStringA("[RainDebug MW] HookedSetTransform VIEW fired\n");
        loggedView = true;
    }
    else if (state == D3DTS_PROJECTION && !loggedProj)
    {
        OutputDebugStringA("[RainDebug MW] HookedSetTransform PROJ fired\n");
        loggedProj = true;
    }
    return g_originalSetTransform ? g_originalSetTransform(device, state, mat) : D3D_OK;
}

static int __cdecl HookedBuildRenderView(void* a0, int a4, int a8)
{
    if (a0 && core::IsReadable(a0, 8))
    {
        int viewIndex = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(a0) + 4);
        if (viewIndex >= 0 && viewIndex < static_cast<int>(WeatherGameAddresses::EViewArrayCount_MW))
        {
            uintptr_t entry = WeatherGameAddresses::EViewArrayBase_MW +
                static_cast<uintptr_t>(viewIndex) * WeatherGameAddresses::EViewSize_MW;
            if (core::IsReadable(reinterpret_cast<void*>(entry), WeatherGameAddresses::EViewCameraOffset_MW + sizeof(void*)))
                PrecipitationController::UpdateMWActiveViewPtr(reinterpret_cast<void*>(entry));
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
        PrecipitationController::UpdateMWViewMatrix(view);
        if (!logged)
        {
            OutputDebugStringA("[RainDebug MW] HookedBuildRenderMatrix fired\n");
            logged = true;
        }
    }
}

static bool InitMinHook()
{
    MH_STATUS status = MH_Initialize();
    return status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED;
}

static uintptr_t GetFeManagerInstanceAddress()
{
    switch (detected_game)
    {
    case GameType::PS: return WeatherGameAddresses::FeManagerInstance_PS;
    case GameType::UC: return WeatherGameAddresses::FeManagerInstance_UC;
    case GameType::CB: return WeatherGameAddresses::FeManagerInstance_CB;
    case GameType::MW: return WeatherGameAddresses::FeManagerInstance_MW;
    default: return 0;
    }
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
    uintptr_t feAddr = GetFeManagerInstanceAddress();
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

static void OnPresent()
{
    if (triedInit)
        return;

    if (IsWeatherInitReady())
    {
        triedInit = true;
        InitializeWeather();
    }
}

static void hk_OnPresent()
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
            OutputDebugStringA("[hk_OnPresent] ⏳ Waiting for camera to become valid...\n");
            alreadyWarned = false;
        }
        else
        {
            if (rainEnabled)
            {
                PrecipitationController::Get()->disable();
                OutputDebugStringA("[hk_OnPresent] 🌤️ Rain disabled by user\n");
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
            OutputDebugStringA("[hk_OnPresent] ✅ Camera now valid, rain enabled\n");
            rainEnabled = true;
            lastKeyState = keyPressed;
            return;
        }

        bool canEnable = false;
        D3DXVECTOR3 cam = PrecipitationController::Get()->GetCameraPositionSafe();
        if (cam != D3DXVECTOR3(0, 0, 0))
            canEnable = true;

        if (!canEnable && detected_game == GameType::MW)
        {
            IDirect3DDevice9* device = PrecipitationController::Get()->m_device;
            if (device)
            {
                D3DXMATRIX view{};
                if (SUCCEEDED(device->GetTransform(D3DTS_VIEW, &view)))
                {
                    D3DXMATRIX invView{};
                    if (D3DXMatrixInverse(&invView, nullptr, &view))
                    {
                        D3DXVECTOR3 pos(invView._41, invView._42, invView._43);
                        if (pos != D3DXVECTOR3(0, 0, 0))
                            canEnable = true;
                    }
                }
            }
        }

        if (canEnable)
        {
            PrecipitationController::Get()->enable();
            OutputDebugStringA("[hk_OnPresent] ✅ Camera now valid, rain enabled\n");
            rainEnabled = true;
        }
        else if (!alreadyWarned)
        {
            OutputDebugStringA("[hk_OnPresent] ⏳ Still waiting for camera...\n");
            alreadyWarned = true;
        }
    }

    if (!shouldEnable && !rainEnabled && RainConfigController::precipitationConfig.enableOnStartup)
    {
        shouldEnable = true;
    }

    lastKeyState = keyPressed;
}

static HRESULT APIENTRY HookedPresent(IDirect3DDevice9* device, CONST RECT* src, CONST RECT* dest, HWND wnd,
                                      CONST RGNDATA* dirty)
{
    if (!PrecipitationController::Get()->m_device)
        PrecipitationController::Get()->m_device = device;

    if (core::useDXVKFix)
        device->BeginScene();

    core::CurrentTime = timeGetTime();

    static bool inCall = false;
    if (inCall)
        return g_originalPresent ? g_originalPresent(device, src, dest, wnd, dirty) : D3D_OK;

    inCall = true;

    if (!g_hookTransferred)
    {
        void** realVTable = *reinterpret_cast<void***>(device);
        void* realPresent = realVTable[17];
        if (realPresent != reinterpret_cast<void*>(&HookedPresent))
        {
            MH_DisableHook(reinterpret_cast<void*>(g_originalDummyPresent));
            MH_RemoveHook(reinterpret_cast<void*>(g_originalDummyPresent));

            PresentFn realOriginal = nullptr;
            MH_CreateHook(realPresent, &HookedPresent, reinterpret_cast<void**>(&realOriginal));
            MH_EnableHook(realPresent);
            g_originalPresent = realOriginal;

            OutputDebugStringA("[HookTransfer] Hooked real game Present\n");
            g_hookTransferred = true;
        }
    }

    OnPresent();
    hk_OnPresent();

    if (PrecipitationController::Get()->IsActive())
        PrecipitationController::Get()->Update();

    HRESULT result = D3D_OK;
    PresentFn toCall = nullptr;

    if (g_originalPresent && g_originalPresent != &HookedPresent)
        toCall = g_originalPresent;
    else if (g_originalDummyPresent && g_originalDummyPresent != &HookedPresent)
        toCall = g_originalDummyPresent;
    else
    {
        static bool warned = false;
        if (!warned)
        {
            OutputDebugStringA("[FATAL] No valid Present target available. Skipping...\n");
            warned = true;
        }
        inCall = false;

        if (core::useDXVKFix)
            device->EndScene();

        return D3D_OK;
    }

    result = toCall(device, src, dest, wnd, dirty);

    // Do not render after Present; precipitation renders before Present.
    inCall = false;

    if (core::useDXVKFix)
        device->EndScene();

    return result;
}

static HRESULT APIENTRY HookedCreateDevice(IDirect3D9* self, UINT adapter, D3DDEVTYPE type, HWND hwnd,
                                           DWORD behavior, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** outDevice)
{
    static thread_local bool inCreateDevice = false;
    if (inCreateDevice)
        return g_originalCreateDevice(self, adapter, type, hwnd, behavior, pp, outDevice);

    OutputDebugStringA("[HookedCreateDevice] Hooking real device Present\n");

    if (g_hookTransferred)
    {
        OutputDebugStringA("[HookedCreateDevice] Hook already transferred, skipping.\n");
        return g_originalCreateDevice(self, adapter, type, hwnd, behavior, pp, outDevice);
    }

    MH_DisableHook(reinterpret_cast<void*>(g_originalCreateDevice));
    MH_RemoveHook(reinterpret_cast<void*>(g_originalCreateDevice));

    inCreateDevice = true;
    HRESULT hr = g_originalCreateDevice(self, adapter, type, hwnd, behavior, pp, outDevice);
    inCreateDevice = false;
    if (FAILED(hr) || !outDevice || !*outDevice)
    {
        OutputDebugStringA("[HookedCreateDevice] [FATAL] CreateDevice failed\n");
        return hr;
    }

    core::useDXVKFix = core::IsDXVKWrapper(*outDevice);

    IDirect3DDevice9* realDevice = *outDevice;
    void** vtable = *reinterpret_cast<void***>(realDevice);
    void* realPresent = vtable[17];
    void* realSetTransform = vtable[44];

    if (realPresent != reinterpret_cast<void*>(&HookedPresent))
    {
        if (g_originalDummyPresent)
        {
            MH_DisableHook(reinterpret_cast<void*>(g_originalDummyPresent));
            MH_RemoveHook(reinterpret_cast<void*>(g_originalDummyPresent));
        }

        if (MH_CreateHook(realPresent, &HookedPresent, reinterpret_cast<void**>(&g_originalPresent)) == MH_OK &&
            MH_EnableHook(realPresent) == MH_OK)
        {
            OutputDebugStringA("[HookedCreateDevice] Hooked real Present successfully\n");
            g_hookTransferred = true;
        }
        else
        {
            OutputDebugStringA("[HookedCreateDevice] [FATAL] Failed to hook real Present\n");
        }
    }

    if (realSetTransform && !g_originalSetTransform)
    {
        if (MH_CreateHook(realSetTransform, &HookedSetTransform, reinterpret_cast<void**>(&g_originalSetTransform)) ==
                MH_OK &&
            MH_EnableHook(realSetTransform) == MH_OK)
        {
            OutputDebugStringA("[HookedCreateDevice] Hooked IDirect3DDevice9::SetTransform\n");
        }
        else
        {
            OutputDebugStringA("[HookedCreateDevice] [WARN] Failed to hook SetTransform\n");
        }
    }

    return hr;
}

static void HookPresent()
{
    if (!InitMinHook())
        return;

    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
        return;

    void** d3dVTable = *reinterpret_cast<void***>(d3d);
    void* createDevice = d3dVTable[16];

    if (MH_CreateHook(createDevice, &HookedCreateDevice, reinterpret_cast<void**>(&g_originalCreateDevice)) == MH_OK)
    {
        MH_EnableHook(createDevice);
        OutputDebugStringA("[HookPresent] Hooked IDirect3D9::CreateDevice\n");
    }

    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = GetForegroundWindow();

    IDirect3DDevice9* dummyDevice = nullptr;
    if (SUCCEEDED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pp.hDeviceWindow,
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dummyDevice)))
    {
        void** dummyVTable = *reinterpret_cast<void***>(dummyDevice);
        void* dummyPresent = dummyVTable[17];

        if (MH_CreateHook(dummyPresent, &HookedPresent, reinterpret_cast<void**>(&g_originalDummyPresent)) == MH_OK)
        {
            MH_EnableHook(dummyPresent);
            OutputDebugStringA("[HookPresent] Hooked dummy Present (fallback)\n");
        }

        dummyDevice->Release();
    }

    d3d->Release();
    core::Initializing();
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
                reinterpret_cast<void*>(WeatherGameAddresses::CreateLookAtAddr_MW),
                &HookedCreateLookAt,
                reinterpret_cast<void**>(&g_originalCreateLookAt));
            MH_EnableHook(reinterpret_cast<void*>(WeatherGameAddresses::CreateLookAtAddr_MW));

            MH_CreateHook(
                reinterpret_cast<void*>(WeatherGameAddresses::BuildViewMatrixAddr_MW),
                &HookedBuildView,
                reinterpret_cast<void**>(&g_originalBuildView));
            MH_EnableHook(reinterpret_cast<void*>(WeatherGameAddresses::BuildViewMatrixAddr_MW));

            MH_CreateHook(
                reinterpret_cast<void*>(WeatherGameAddresses::BuildRenderViewAddr_MW),
                &HookedBuildRenderView,
                reinterpret_cast<void**>(&g_originalBuildRenderView));
            MH_EnableHook(reinterpret_cast<void*>(WeatherGameAddresses::BuildRenderViewAddr_MW));

            MH_CreateHook(
                reinterpret_cast<void*>(WeatherGameAddresses::BuildRenderMatrixAddr_MW),
                &HookedBuildRenderMatrix,
                reinterpret_cast<void**>(&g_originalBuildRenderMatrix));
            MH_EnableHook(reinterpret_cast<void*>(WeatherGameAddresses::BuildRenderMatrixAddr_MW));

            MH_CreateHook(
                reinterpret_cast<void*>(0x006DE300),
                &HookedDisplayFrameMW,
                reinterpret_cast<void**>(&g_originalDisplayFrameMW));
            MH_EnableHook(reinterpret_cast<void*>(0x006DE300));

            injector::MakeCALL(0x006DF545, HookedRainTickMW, true);

            MH_CreateHook(
                reinterpret_cast<void*>(WeatherGameAddresses::RainRender_MW),
                &HookedRainRenderMW,
                reinterpret_cast<void**>(&g_originalRainRenderMW));
            MH_EnableHook(reinterpret_cast<void*>(WeatherGameAddresses::RainRender_MW));
        }
    }

    HookPresent();
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
