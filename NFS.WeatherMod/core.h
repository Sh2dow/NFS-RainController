#pragma once
#include <windows.h>
#include <functional>

struct IDirect3DDevice9;

namespace core
{
    bool IsReadable(void* ptr, size_t size);

    inline static float fpsDeltaTime;
    inline static DWORD CurrentTime = 0;
    
    
    inline uint8_t* Ptr(uintptr_t addr) { return reinterpret_cast<uint8_t*>(addr); }
    inline float* FPtr(uintptr_t addr) { return reinterpret_cast<float*>(addr); }
    inline int* IPtr(uintptr_t addr) { return reinterpret_cast<int*>(addr); }
}
