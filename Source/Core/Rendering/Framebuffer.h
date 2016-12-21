#pragma once

#include <memory>
#include <unordered_map>

#include "Core/Resource.h"

struct FramebufferCreateInfo;
struct Image;

struct Framebuffer : Resource
{
	enum class Attachment
	{
		//TODO: More color attachments
		COLOR0,
		DEPTH,
		STENCIL,
		DEPTH_STENCIL
	};

	struct AttachmentHash
	{
		constexpr std::size_t operator()(Attachment a) const
		{
			return static_cast<size_t>(a);
		}
	};

	Framebuffer(ResourceManager *, const std::string&);
	Framebuffer(const FramebufferCreateInfo&);
	//TODO: Doesn't really make sense.
	Framebuffer(ResourceManager *, const std::string&, const std::vector<char>&);

	const std::unordered_map<Framebuffer::Attachment, std::shared_ptr<Image>, AttachmentHash>& GetImages() const;
	void * GetRendererData() const;

	void SetRendererData(void *);

private:
	//Images the framebuffer renders into, e.g. color, depth, G-bufs, etc.
	std::unordered_map<Framebuffer::Attachment, std::shared_ptr<Image>, AttachmentHash> imgs;
	void * rendererData = nullptr;
};

struct FramebufferCreateInfo
{
	std::string name;
	std::unordered_map<Framebuffer::Attachment, std::shared_ptr<Image>, Framebuffer::AttachmentHash> imgs;
	//If rendererData is null, the constructor creates a new rendererData
	//If not null, it's presumed the user already created a correct rendererData for the imgs and is passing it in
	void * rendererData = nullptr;
};

RESOURCE_HEADER(Framebuffer)
