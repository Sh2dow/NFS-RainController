#include <windows.h>
#include "core.h"
#include <thread>

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