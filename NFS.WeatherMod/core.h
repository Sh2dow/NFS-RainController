#pragma once
#include <windows.h>
#include <d3dx9.h>
#include <functional>

struct IDirect3DDevice9;

namespace core
{
    bool IsReadable(void* ptr, size_t size);

    inline static float fpsDeltaTime;
    inline static DWORD CurrentTime = 0;
}
