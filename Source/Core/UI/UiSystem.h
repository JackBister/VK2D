#pragma once

class ResourceManager;

namespace UiSystem
{
	void BeginFrame();
	void EndFrame();
	void Init(ResourceManager * resourceManager);
}
