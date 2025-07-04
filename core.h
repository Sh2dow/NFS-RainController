#pragma once
#include <windows.h>
#include <d3dx9.h>
#include <functional>

struct IDirect3DDevice9;

namespace core
{
    bool IsReadable(void* ptr, size_t size);

    inline static float fpsDeltaTime;
    bool IsDXVKWrapper(IDirect3DDevice9* device);
    using VoidCallback = std::function<void()>;
    using D3DCallback = std::function<void(IDirect3DDevice9*)>;

    void AddLoop(VoidCallback cb);
    size_t AddDirectX9Loop(D3DCallback cb);
    void CallDirectX9Callbacks(IDirect3DDevice9* device);
    void RemoveDirectX9Loop(size_t id);
    void Initializing();
    static __readonly bool useDXVKFix;
    inline static DWORD CurrentTime = 0;
}
