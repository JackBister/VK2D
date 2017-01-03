#include "Core/Rendering/Framebuffer.h"

#include "Core/Rendering/Image.h"
#include "Core/ResourceManager.h"

Framebuffer::Framebuffer(ResourceManager * resMan, const std::string& name) : imgs({{Framebuffer::Attachment::COLOR0, resMan->LoadResourceRefCounted<Image>(name + ".COLOR0.tex")}})
{
	this->name = name;
	RenderCommand rc(RenderCommand::AddFramebufferParams(this));
	resMan->PushRenderCommand(rc);
}

Framebuffer::Framebuffer(const FramebufferCreateInfo& createInfo) : imgs(createInfo.imgs)
{
	this->name = createInfo.name;

	if (createInfo.rendererData.index() == 0) {
		RenderCommand rc(RenderCommand::AddFramebufferParams(this));
		createInfo.resMan->PushRenderCommand(rc);
	} else {
		rendererData = std::experimental::get<FramebufferRendererData>(createInfo.rendererData);
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

const FramebufferRendererData& Framebuffer::GetRendererData() const
{
	return rendererData;
}

void Framebuffer::SetRendererData(const FramebufferRendererData& rData)
{
	rendererData = rData;
}
