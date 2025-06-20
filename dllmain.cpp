// Minimal reimplementation of the NFS-RainController plugin.
#include <windows.h>
#include <vector>
#include <memory>
#include <d3dx9.h>
#include "core.h"
#include "features.h"
#include "ForcePrecipitation.h"
#include "RainConfigController.h"
#include "minhook/include/MinHook.h"

#ifdef GAME_PS
#include "NFSPS_PreFEngHook.h"
#elif GAME_UC
#include "NFSUC_PreFEngHook.h"
#endif

using namespace ngg::common;

// TODO: Port full initialization logic from sub_10077220
static std::vector<std::unique_ptr<ngg::common::Feature>> g_features;

static void SetupFeatures()
{
    g_features.emplace_back(std::make_unique<ForcePrecipitation>());
}

bool triedInit = false;

// Original Present function pointer
typedef HRESULT (APIENTRY*PresentFn)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
PresentFn g_originalPresent = nullptr;

static void Initialize()
{
    using namespace ngg::common::features;

    if (g_features.empty())
        SetupFeatures();

    OutputDebugStringA("[Initialize] Setting up hooks\n");

    RainConfigController::Load(); // Load INI config once at startup 

    for (const auto& feature : g_features)
    {
        const char* name = feature->name();
        OutputDebugStringA(name);
        OutputDebugStringA(" initialized\n");

        if (strcmp(name, "ForcePrecipitation") == 0)
        {
            if (RainConfigController::enabled)
            {
                feature->enable();
                OutputDebugStringA("[Initialize] Precipitation enabled\n");
            }
            else
            {
                ForcePrecipitation::Get()->PatchRainSettings(0); // Static! Doesn't touch .Get()
                OutputDebugStringA("[Initialize] Precipitation disabled in config\n");
            }
        }
        else
        {
            feature->enable(); // regular features
        }
    }
}

void OnPresent()
{
    if (triedInit)
        return;

    auto feManager = *reinterpret_cast<void**>(FEMANAGER_INSTANCE_ADDR);
    if (feManager && !IsBadReadPtr(feManager, 0x40))
    {
        triedInit = true;
        Initialize();
    }
}

void hk_OnPresent()
{
    static bool lastState = false;
    bool pressed = (GetAsyncKeyState(RainConfigController::toggleKey) & 0x8000) != 0;

    static bool toggled = false;
    if (pressed && !lastState)
    {
        toggled = !toggled;

        if (toggled)
            ForcePrecipitation::Get()->enable();
        else
            ForcePrecipitation::Get()->disable();

        OutputDebugStringA(toggled ? "[hk_OnPresent] Rain enabled\n" : "[hk_OnPresent] Rain disabled\n");
    }

    lastState = pressed;
}

// Hooked Present
HRESULT APIENTRY HookedPresent(IDirect3DDevice9* device, CONST RECT* src, CONST RECT* dest, HWND wnd,
                               CONST RGNDATA* dirty)
{
    OnPresent(); // <- trigger Initialize logic
    hk_OnPresent(); // Toggle state BEFORE rendering
    core::CallDirectX9Callbacks(device); // ← your D3D drawing (rain, overlays, etc.)

    return g_originalPresent(device, src, dest, wnd, dirty);
}

void HookPresent()
{
    // Create dummy device to get vtable
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS pp = {};
    pp.Windowed = TRUE;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = GetForegroundWindow();

    IDirect3DDevice9* dummyDevice = nullptr;
    if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, pp.hDeviceWindow,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &dummyDevice)))
    {
        d3d->Release();
        return;
    }

    void** vtable = *reinterpret_cast<void***>(dummyDevice);
    g_originalPresent = reinterpret_cast<PresentFn>(vtable[17]); // Present is index 17

    MH_Initialize();
    MH_CreateHook(vtable[17], &HookedPresent, reinterpret_cast<void**>(&g_originalPresent));
    MH_EnableHook(vtable[17]);

    dummyDevice->Release();
    d3d->Release();

    core::Initializing();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        CreateThread(nullptr, 0, [](LPVOID) -> DWORD
        {
            // while (!ngg::common::GetExeBase())
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));

            HookPresent(); // <--- Just hook Present, defer actual init
            return 0;
        }, nullptr, 0, nullptr);
    }

    return TRUE;
}
