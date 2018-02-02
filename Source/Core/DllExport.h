#pragma once

#ifdef VK2D_DLL
#define EAPI __declspec(dllimport)
#else
#define EAPI __declspec(dllexport)
#endif

#ifdef __INTELLISENSE__
#define EAPI
#endif
