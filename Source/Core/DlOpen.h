#pragma once

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
typedef HMODULE DllModule;
#else
#include <dlfcn.h>
typedef void* DllModule;
#endif

DllModule DlOpen(std::string const& filename);

void * DlSym(DllModule handle, std::string const& name);

void DlClose(DllModule handle);
