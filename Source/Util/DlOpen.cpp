#include "DlOpen.h"

#include <codecvt>
#include <locale>
#include <string>

DllModule DlOpen(std::string const & filename)
{
#ifdef _WIN32
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    auto temp = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(filename);
    return LoadLibrary(temp.c_str());
#else
    return dlopen(filename.c_str(), RTLD_NOW);
#endif
}

void * DlSym(DllModule handle, std::string const & name)
{
#ifdef _WIN32
    return GetProcAddress(handle, name.c_str());
#else
    return dlsym(handle, name.c_str());
#endif
}

void DlClose(DllModule handle)
{
#ifdef _WIN32
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}
