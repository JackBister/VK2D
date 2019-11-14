#include "Core/GameModule.h"

#include "Core/Components/Component.h"
#include "Core/DlOpen.h"
#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("GameModule");

namespace GameModule {
	using LoadComponentsFunc = void(*)();
	using UnloadComponentsFunc = void(*)();

	std::unordered_map<std::string, DllModule> loadedModules;

	void LoadDLL(std::string const& filename)
	{
		auto handle = DlOpen(filename);
		if (handle == nullptr) {
			logger->Errorf("LoadDLL: DlOpen failed.");
			return;
		}
		loadedModules[filename] = handle;
		auto loadComponents = (LoadComponentsFunc)DlSym(handle, "LoadComponents");
		if (loadComponents == nullptr) {
			logger->Errorf("LoadDLL: LoadComponents not found.");
			return;
		}
		loadComponents();
	}

	void UnloadDLL(std::string const& filename)
	{
		if (loadedModules.find(filename) == loadedModules.end()) {
			logger->Errorf("UnloadDLL: Module not found.");
			return;
		}
		auto module = loadedModules[filename];

		auto unloadComponents = (UnloadComponentsFunc)DlSym(module, "UnloadComponents");
		if (unloadComponents == nullptr) {
			logger->Errorf("UnloadDLL: UnloadComponents not found.");
			return;
		}

		unloadComponents();
		DlClose(module);
	}
};
