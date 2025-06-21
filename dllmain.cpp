// DXVK-compatible rain plugin patch for NFS ProStreet / UC with CreateDevice hook fallback
#include <windows.h>
#include <vector>
#include <memory>
#include <d3dx9.h>
#include "core.h"
#include "features.h"
#include "PrecipitationController.h"
#include "RainConfigController.h"
#include "minhook/include/MinHook.h"

#ifdef GAME_PS
#include "NFSPS_PreFEngHook.h"
#elif GAME_UC
#include "NFSUC_PreFEngHook.h"
#endif

using namespace ngg::common;

static std::vector<std::unique_ptr<ngg::common::Feature>> g_features;
static bool g_hookTransferred = false;
static bool triedInit = false;

typedef HRESULT (APIENTRY*PresentFn)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT (APIENTRY*CreateDeviceFn)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*,
                                          IDirect3DDevice9**);

PresentFn g_originalPresent = nullptr;
PresentFn g_originalDummyPresent = nullptr;
CreateDeviceFn g_originalCreateDevice = nullptr;

static void SetupFeatures()
{
    g_features.emplace_back(std::make_unique<PrecipitationController>());
}

static void Initialize()
{
    if (g_features.empty())
        SetupFeatures();

    OutputDebugStringA("[Initialize] Setting up hooks\n");

    RainConfigController::LoadOnStartup();

    for (const auto& feature : g_features)
    {
        const char* name = feature->name();
        OutputDebugStringA(name);
        OutputDebugStringA(" initialized\n");

        if (strcmp(name, "PrecipitationController") == 0)
        {
            if (RainConfigController::g_precipitationConfig.enableOnStartup)
            {
                feature->enable();
                OutputDebugStringA("[Initialize] Precipitation enabled\n");
            }
            else
            {
                feature->disable();
                OutputDebugStringA("[Initialize] Precipitation disabled in config\n");
            }
        }
        else
        {
            feature->enable();
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
            PrecipitationController::Get()->enable();
        else
            PrecipitationController::Get()->disable();

        OutputDebugStringA(toggled ? "[hk_OnPresent] Rain enabled\n" : "[hk_OnPresent] Rain disabled\n");
    }

    lastState = pressed;
}

HRESULT APIENTRY HookedPresent(IDirect3DDevice9* device, CONST RECT* src, CONST RECT* dest, HWND wnd,
                               CONST RGNDATA* dirty)
{
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
    {
        PrecipitationController::Get()->Update(device); // draw rain last
    }

    HRESULT result = D3D_OK;
    PresentFn toCall = nullptr;

    if (g_originalPresent && g_originalPresent != &HookedPresent)
    {
        toCall = g_originalPresent;
    }
    else if (g_originalDummyPresent && g_originalDummyPresent != &HookedPresent)
    {
        toCall = g_originalDummyPresent;
    }
    else
    {
        // Avoid spamming fatal unless it's truly unexpected
        static bool warned = false;
        if (!warned)
        {
            OutputDebugStringA("[FATAL] No valid Present target available. Skipping...\n");
            warned = true;
        }
        inCall = false;
        return D3D_OK; // Safe fallback to avoid infinite loop
    }

    result = toCall(device, src, dest, wnd, dirty);

    core::CallDirectX9Callbacks(device);
    inCall = false;
    return result;
}

void HookPresentFromRuntimeDevice(IDirect3DDevice9* device)
{
    void** vtable = *reinterpret_cast<void***>(device);
    void* realPresent = vtable[17];

    PresentFn realOriginal = nullptr;
    MH_CreateHook(realPresent, &HookedPresent, reinterpret_cast<void**>(&realOriginal));
    MH_EnableHook(realPresent);

    g_originalPresent = realOriginal; // save actual original
    OutputDebugStringA("[HookPresentFromRuntimeDevice] Hooked real Present from game device\n");
}

HRESULT APIENTRY HookedCreateDevice(IDirect3D9* self, UINT adapter, D3DDEVTYPE type, HWND hwnd,
                                    DWORD behavior, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** outDevice)
{
    OutputDebugStringA("[HookedCreateDevice] Hooking real device Present\n");

    if (g_hookTransferred)
    {
        OutputDebugStringA("[HookedCreateDevice] Hook already transferred, skipping.\n");
        return g_originalCreateDevice(self, adapter, type, hwnd, behavior, pp, outDevice);
    }

    // Temporarily unhook ourselves to avoid recursion
    MH_DisableHook(reinterpret_cast<void*>(g_originalCreateDevice));
    MH_RemoveHook(reinterpret_cast<void*>(g_originalCreateDevice));

    // Call the original CreateDevice
    HRESULT hr = g_originalCreateDevice(self, adapter, type, hwnd, behavior, pp, outDevice);
    if (FAILED(hr) || !outDevice || !*outDevice)
    {
        OutputDebugStringA("[HookedCreateDevice] [FATAL] CreateDevice failed\n");
        return hr;
    }

    IDirect3DDevice9* realDevice = *outDevice;
    void** vtable = *reinterpret_cast<void***>(realDevice);
    void* realPresent = vtable[17];

    if (realPresent != reinterpret_cast<void*>(&HookedPresent))
    {
        // Disable dummy hook if present
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

    return hr;
}

void HookPresent()
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return;

    // Hook CreateDevice (vtable[16]) to detect when real game device is created
    void** d3dVTable = *reinterpret_cast<void***>(d3d);
    void* createDevice = d3dVTable[16];

    if (MH_CreateHook(createDevice, &HookedCreateDevice, reinterpret_cast<void**>(&g_originalCreateDevice)) == MH_OK)
    {
        MH_EnableHook(createDevice);
        OutputDebugStringA("[HookPresent] Hooked IDirect3D9::CreateDevice\n");
    }

    // Setup dummy Present hook (used until the real device is available)
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

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        MH_Initialize();
        DisableThreadLibraryCalls(hModule);

        core::useDXVKFix = core::IsDXVKWrapper();
        
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD
        {
            HookPresent();
            return 0;
        }, nullptr, 0, nullptr);
    }

    return TRUE;
}
