#include "Core/Rendering/Framebuffer.h"

#include "Core/Rendering/Image.h"
#include "Core/ResourceManager.h"

Framebuffer::Framebuffer(ResourceManager * resMan, const std::string& name) : imgs({{Framebuffer::Attachment::COLOR0, resMan->LoadResource<Image>(name + ".COLOR0.tex")}})
{
	this->name = name;
	resMan->PushRenderCommand(RenderCommand(RenderCommand::AddFramebufferParams(this)));
}

Framebuffer::Framebuffer(const FramebufferCreateInfo& createInfo) : imgs(createInfo.imgs)
{
	this->name = createInfo.name;

	if (createInfo.rendererData.index() == 0) {
		createInfo.resMan->PushRenderCommand(RenderCommand(RenderCommand::AddFramebufferParams(this)));
	} else {
		rendererData = std::get<FramebufferRendererData>(createInfo.rendererData);
	}
}

Framebuffer::Framebuffer(ResourceManager * _, const std::string& name, const std::vector<uint8_t>& input)
{
	this->name = name;
}

const std::unordered_map<Framebuffer::Attachment, std::shared_ptr<Image>, Framebuffer::AttachmentHash>& Framebuffer::GetImages() const
{
	return imgs;
}

const FramebufferRendererData& Framebuffer::GetRendererData() const
{
	return rendererData;
}

void Framebuffer::SetRendererData(const FramebufferRendererData& rData)
{
	rendererData = rData;
}
