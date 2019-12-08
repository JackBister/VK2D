#include "SetThreadName.h"

#include <optick/optick.h>

#if defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

void SetThreadName(std::thread::id id, std::string const & name)
{
#ifdef _WIN64
#pragma pack(push, 8)
    typedef struct tagTHREADNAME_INFO {
        DWORD dwType;     // Must be 0x1000.
        LPCSTR szName;    // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags;    // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    const DWORD MS_VC_EXCEPTION = 0x406D1388;
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name.c_str();
    info.dwThreadID = *(DWORD *)&id;
    info.dwFlags = 0;

    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#endif
}
