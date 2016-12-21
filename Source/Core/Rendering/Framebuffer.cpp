#include "Core/Rendering/Framebuffer.h"

#include "Core/Rendering/Image.h"
#include "Core/Rendering/render.h"
#include "Core/ResourceManager.h"

Framebuffer::Framebuffer(ResourceManager * resMan, const std::string& name) : imgs({{Framebuffer::Attachment::COLOR0, resMan->LoadResourceRefCounted<Image>(name + ".COLOR0.tex")}})
{
	this->name = name;
	Render_currentRenderer->AddFramebuffer(this);
}

Framebuffer::Framebuffer(const FramebufferCreateInfo& createInfo) : imgs(createInfo.imgs),  rendererData(createInfo.rendererData)
{
	this->name = createInfo.name;

	if (createInfo.rendererData == nullptr) {
		Render_currentRenderer->AddFramebuffer(this);
	}
}

Framebuffer::Framebuffer(ResourceManager * _, const std::string& name, const std::vector<char>& input)
{
	this->name = name;
}

const std::unordered_map<Framebuffer::Attachment, std::shared_ptr<Image>, Framebuffer::AttachmentHash>& Framebuffer::GetImages() const
{
	return imgs;
}

void * Framebuffer::GetRendererData() const
{
	return rendererData;
}

void Framebuffer::SetRendererData(void *rData)
{
	rendererData = rData;
}
