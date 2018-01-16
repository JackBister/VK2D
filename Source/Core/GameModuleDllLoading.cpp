#include "Core/GameModule.h"

#include "Core/Components/Component.h"
#include "Core/DlOpen.h"

namespace GameModule {
	using LoadComponentsFunc = void(*)();
	using UnloadComponentsFunc = void(*)();

	std::unordered_map<std::string, DllModule> loadedModules;

	void LoadDLL(std::string const& filename)
	{
		auto handle = DlOpen(filename);
		if (handle == nullptr) {
			printf("[ERROR] LoadDLL: DlOpen failed.\n");
			return;
		}
		loadedModules[filename] = handle;
		auto loadComponents = (LoadComponentsFunc)DlSym(handle, "LoadComponents");
		if (loadComponents == nullptr) {
			printf("[ERROR] LoadDLL: LoadComponents not found.\n");
			return;
		}
		loadComponents();
	}

	void UnloadDLL(std::string const& filename)
	{
		if (loadedModules.find(filename) == loadedModules.end()) {
			printf("[ERROR] UnloadDLL: Module not found.");
			return;
		}
		auto module = loadedModules[filename];

		auto unloadComponents = (UnloadComponentsFunc)DlSym(module, "UnloadComponents");
		if (unloadComponents == nullptr) {
			printf("[ERROR] UnloadDLL: UnloadComponents not found.\n");
			return;
		}

		unloadComponents();
		DlClose(module);
	}
};
