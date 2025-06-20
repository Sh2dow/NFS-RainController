// DXVK-compatible rain plugin patch for NFS ProStreet / UC
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

static std::vector<std::unique_ptr<ngg::common::Feature>> g_features;
static bool g_hookTransferred = false;
static bool triedInit = false;
typedef HRESULT (APIENTRY*PresentFn)(IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
PresentFn g_originalPresent = nullptr;
PresentFn g_originalDummyPresent = nullptr;

static void SetupFeatures()
{
    g_features.emplace_back(std::make_unique<ForcePrecipitation>());
}

static void Initialize()
{
    if (g_features.empty())
        SetupFeatures();

    OutputDebugStringA("[Initialize] Setting up hooks\n");

    RainConfigController::Load();

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
            ForcePrecipitation::Get()->enable();
        else
            ForcePrecipitation::Get()->disable();

        OutputDebugStringA(toggled ? "[hk_OnPresent] Rain enabled\n" : "[hk_OnPresent] Rain disabled\n");
    }

    lastState = pressed;
}

HRESULT APIENTRY HookedPresent(IDirect3DDevice9* device, CONST RECT* src, CONST RECT* dest, HWND wnd, CONST RGNDATA* dirty)
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

    if (ForcePrecipitation::Get()->IsActive())
    {
        ForcePrecipitation::Get()->Update(device); // draw rain last
    }

    HRESULT result = D3D_OK;
    PresentFn toCall = (g_hookTransferred && g_originalPresent != &HookedPresent)
        ? g_originalPresent
        : (g_originalDummyPresent != &HookedPresent ? g_originalDummyPresent : nullptr);

    if (toCall)
        result = toCall(device, src, dest, wnd, dirty);
    else
        OutputDebugStringA("[!!] Skipping Present to avoid recursion\n");

    core::CallDirectX9Callbacks(device);
    inCall = false;
    return result;
}

void HookPresentFromRuntimeDevice(IDirect3DDevice9* device)
{
    void** vtable = *reinterpret_cast<void***>(device);
    g_originalPresent = reinterpret_cast<PresentFn>(vtable[17]);

    MH_Initialize();
    MH_CreateHook(vtable[17], &HookedPresent, reinterpret_cast<void**>(&g_originalPresent));
    MH_EnableHook(vtable[17]);

    OutputDebugStringA("[RainController] Hooked real Present from game device\n");
}

void HookPresent()
{
    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return;

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
    void* dummyPresent = vtable[17];

    g_originalDummyPresent = reinterpret_cast<PresentFn>(dummyPresent);
    MH_CreateHook(dummyPresent, &HookedPresent, nullptr);
    MH_EnableHook(dummyPresent);

    dummyDevice->Release();
    d3d->Release();

    core::Initializing();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        MH_Initialize();
        DisableThreadLibraryCalls(hModule);

        CreateThread(nullptr, 0, [](LPVOID) -> DWORD
        {
            HookPresent();
            return 0;
        }, nullptr, 0, nullptr);
    }

    return TRUE;
}
