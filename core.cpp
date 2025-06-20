#include <windows.h>
#include "core.h"
#include <vector>
#include <mutex>
#include <thread>

static std::vector<core::VoidCallback> loopCallbacks;
static std::vector<core::D3DCallback> d3dCallbacks;
static std::mutex coreMutex;
static bool running = true;

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
