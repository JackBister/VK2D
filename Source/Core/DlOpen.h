#pragma once

#include <string>

#ifdef _WIN32
#include <Windows.h>
typedef HMODULE DllModule;
#else
#include <dlfcn.h>
typedef void* DllModule;
#endif

DllModule DlOpen(std::string const& filename);

void * DlSym(DllModule handle, std::string const& name);

void DlClose(DllModule handle);
