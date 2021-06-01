#include "DllManager.h"

#include "Core/Logging/Logger.h"
#include "Util/DlOpen.h"

static auto const logger = Logger::Create("DllManager");

struct LoadedDllModule {
    DllModule dllModule;
};

DllManager * DllManager::GetInstance()
{
    static DllManager singletonDllManager;
    return &singletonDllManager;
}

void DllManager::LoadDll(std::filesystem::path dllPath)
{
    auto handle = DlOpen(dllPath.string());
    if (handle == nullptr) {
        logger->Errorf("LoadDLL: DlOpen failed.");
        return;
    }
    auto loadComponents = (LoadComponentsFunc)DlSym(handle, "LoadComponents");
    if (loadComponents == nullptr) {
        logger->Errorf("LoadDLL: LoadComponents not found.");
        return;
    }
    auto unloadComponents = (UnloadComponentsFunc)DlSym(handle, "UnloadComponents");
    if (unloadComponents == nullptr) {
        logger->Errorf("UnloadDLL: UnloadComponents not found.");
        return;
    }
    loadComponents();

    loadedDlls.push_back(LoadedDll{
        .path = dllPath, .dllModule = new LoadedDllModule{.dllModule = handle}, .unloadComponents = unloadComponents});
}

void DllManager::UnloadAllDlls()
{
    for (auto const & dll : loadedDlls) {
        dll.unloadComponents();
        DlClose(dll.dllModule->dllModule);
        delete dll.dllModule;
    }
    loadedDlls.clear();
}