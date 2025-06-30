#include "stdafx.h"
#include <windows.h>
#include "core.h"
#include <vector>
#include <mutex>
#include <thread>
static std::vector<core::VoidCallback> loopCallbacks;
static std::vector<core::D3DCallback> d3dCallbacks;
static std::mutex coreMutex;
static bool running = true;

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


bool core::IsReadable(void* ptr, size_t size = 1)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(ptr, &mbi, sizeof(mbi)))
    {
        bool committed = mbi.State == MEM_COMMIT;
        bool readable = (mbi.Protect & PAGE_READONLY) ||
                        (mbi.Protect & PAGE_READWRITE) ||
                        (mbi.Protect & PAGE_EXECUTE_READ) ||
                        (mbi.Protect & PAGE_EXECUTE_READWRITE);
        return committed && readable && (uintptr_t(ptr) + size <= uintptr_t(mbi.BaseAddress) + mbi.RegionSize);
    }
    return false;
}

bool core::IsDXVKWrapper(IDirect3DDevice9* device)
{
    if (!device)
    {
        OutputDebugStringA("[IsDXVKWrapper] device is null\n");
        return false;
    }

    IDirect3D9* d3d = nullptr;
    if (FAILED(device->GetDirect3D(&d3d)) || !d3d)
    {
        OutputDebugStringA("[IsDXVKWrapper] GetDirect3D failed\n");
        return false;
    }

    D3DADAPTER_IDENTIFIER9 ident = {};
    if (SUCCEEDED(d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &ident)))
    {
        OutputDebugStringA("[IsDXVKWrapper] Adapter description: ");
        OutputDebugStringA(ident.Description);
        OutputDebugStringA("\n");

        if (strstr(ident.Description, "DXVK") != nullptr)
        {
            OutputDebugStringA("[IsDXVKWrapper] ✅ Detected DXVK via adapter string.\n");
            d3d->Release();
            return true;
        }
    }

    d3d->Release();

    // Optional fallback: check for vulkan-1.dll
    if (GetModuleHandleA("vulkan-1.dll"))
    {
        OutputDebugStringA("[IsDXVKWrapper] Vulkan loaded — possible DXVK\n");
        return true; // Use caution here
    }

    OutputDebugStringA("[IsDXVKWrapper] ❌ Not DXVK\n");
    return false;
}

void core::AddLoop(VoidCallback cb)
{
    std::scoped_lock lock(coreMutex);
    loopCallbacks.push_back(cb);
}

static void CoreLoop()
{
    while (running)
    {
        {
            std::scoped_lock lock(coreMutex);
            for (auto& cb : loopCallbacks)
                cb();
            // You could optionally call D3D callbacks here if you hook Present
        }
        Sleep(16);
    }
}

size_t core::AddDirectX9Loop(D3DCallback cb)
{
    std::scoped_lock lock(coreMutex);
    d3dCallbacks.push_back(cb);
    return d3dCallbacks.size() - 1; // store this in m_callbackId
}

void core::CallDirectX9Callbacks(IDirect3DDevice9* device)
{
    std::scoped_lock lock(coreMutex);
    for (auto& cb : d3dCallbacks)
        cb(device);
}

void core::RemoveDirectX9Loop(size_t id)
{
    std::scoped_lock lock(coreMutex);
    if (id < d3dCallbacks.size())
        d3dCallbacks.erase(d3dCallbacks.begin() + id);
}

void core::Initializing()
{
    std::thread(CoreLoop).detach();
}
