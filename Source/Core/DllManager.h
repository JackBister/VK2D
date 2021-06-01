#pragma once

#include <filesystem>
#include <vector>

struct LoadedDllModule;

using LoadComponentsFunc = void (*)();
using UnloadComponentsFunc = void (*)();

struct LoadedDll {
    std::filesystem::path path;
    LoadedDllModule * dllModule;
    UnloadComponentsFunc unloadComponents;
};

class DllManager
{
public:
    static DllManager * GetInstance();

    void LoadDll(std::filesystem::path dllPath);
    void UnloadAllDlls();

private:
    std::vector<LoadedDll> loadedDlls;
};
