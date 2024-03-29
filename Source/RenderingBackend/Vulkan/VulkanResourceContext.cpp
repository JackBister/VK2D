#ifndef USE_OGL_RENDERER
#include "VulkanResourceContext.h"

#include <ThirdParty/optick/src/optick.h>

#include "Jobs/JobEngine.h"
#include "Logging/Logger.h"
#include "RenderingBackend/Abstract/SpecializationConstants.h"
#include "VulkanCommandBufferAllocator.h"
#include "VulkanContextStructs.h"
#include "VulkanConverterFuncs.h"
#include "VulkanRenderer.h"

static const auto logger = Logger::Create("VulkanResourceContext");

CommandBufferAllocator * VulkanResourceContext::CreateCommandBufferAllocator()
{
    auto mem = allocator.allocate(sizeof(VulkanCommandBufferAllocator));
    auto ret = new (mem) VulkanCommandBufferAllocator();
    ret->device = renderer->basics.device;
    ret->renderer = renderer;
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = renderer->graphicsQueueIdx;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(renderer->basics.device, &createInfo, nullptr, &ret->commandPool);
    return ret;
}

void VulkanResourceContext::DestroyCommandBufferAllocator(CommandBufferAllocator * pool)
{
    assert(pool != nullptr);
    auto nativeHandle = (VulkanCommandBufferAllocator *)pool;
    vkDestroyCommandPool(renderer->basics.device, nativeHandle->commandPool, nullptr);
}

void VulkanResourceContext::BufferSubData(BufferHandle * buffer, uint8_t * data, size_t offset, size_t size)
{
    OPTICK_EVENT();
    auto const nativeHandle = (VulkanBufferHandle *)buffer;

    auto stagingBuffer = renderer->GetStagingBuffer(size);

    uint8_t * const mappedMemory = [this](VkDeviceMemory const & memory, size_t size) {
        uint8_t * ret = nullptr;
        auto const res = vkMapMemory(renderer->basics.device, memory, 0, size, 0, (void **)&ret);
        assert(res == VK_SUCCESS);
        return ret;
    }(stagingBuffer.buffer->memory, size);

    memcpy(mappedMemory, data, size);

    vkUnmapMemory(renderer->basics.device, stagingBuffer.buffer->memory);

    auto cb = renderer->GetStagingCommandBuffer();
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    {
        OPTICK_EVENT("RecordTransferCommands");
        vkBeginCommandBuffer(cb.commandBuffer, &beginInfo);
        renderer->CopyBufferToBuffer(
            cb.commandBuffer, stagingBuffer.buffer->buffer, nativeHandle->buffer, offset, size);
        vkEndCommandBuffer(cb.commandBuffer);
    }

    VkFence tempFence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(renderer->basics.device, &fenceInfo, nullptr, &tempFence);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb.commandBuffer;
    // TODO: Should submit and return a fence here that the user can use to know that their data has uploaded instead of
    // using vkQueueWaitIdle
    {
        auto queue = renderer->GetTransferQueue();
        vkQueueSubmit(queue.queue, 1, &submitInfo, cb.isReady);
        VkSubmitInfo dummySubmit = {};
        dummySubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        dummySubmit.commandBufferCount = 0;
        vkQueueSubmit(queue.queue, 1, &dummySubmit, tempFence);
    }
    {
        OPTICK_EVENT("FenceWait")
        vkWaitForFences(renderer->basics.device, 1, &tempFence, VK_TRUE, UINT64_MAX);
    }
    vkDestroyFence(renderer->basics.device, tempFence, nullptr);
}

BufferHandle * VulkanResourceContext::CreateBuffer(BufferCreateInfo const & ci)
{
    auto const ret = (VulkanBufferHandle *)allocator.allocate(sizeof(VulkanBufferHandle));
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = ci.size;
    bufferInfo.usage = ci.usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto res = vkCreateBuffer(renderer->basics.device, &bufferInfo, nullptr, &ret->buffer);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(renderer->basics.device, ret->buffer, &memRequirements);

    VkMemoryAllocateInfo memInfo = {};
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.allocationSize = memRequirements.size;
    memInfo.memoryTypeIndex = renderer->FindMemoryType(memRequirements.memoryTypeBits, ci.memoryProperties);

    res = vkAllocateMemory(renderer->basics.device, &memInfo, nullptr, &ret->memory);
    assert(res == VK_SUCCESS);

    res = vkBindBufferMemory(renderer->basics.device, ret->buffer, ret->memory, 0);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroyBuffer(BufferHandle * buffer)
{
    assert(buffer != nullptr);

    auto nativeHandle = (VulkanBufferHandle *)buffer;

    vkDestroyBuffer(renderer->basics.device, nativeHandle->buffer, nullptr);
    vkFreeMemory(renderer->basics.device, nativeHandle->memory, nullptr);
    allocator.deallocate((uint8_t *)buffer, sizeof(VulkanBufferHandle));
}

uint8_t * VulkanResourceContext::MapBuffer(BufferHandle * buffer, size_t offset, size_t size)
{
    assert(buffer != nullptr);

    uint8_t * ret;
    auto const res =
        vkMapMemory(renderer->basics.device, ((VulkanBufferHandle *)buffer)->memory, offset, size, 0, (void **)&ret);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::UnmapBuffer(BufferHandle * buffer)
{
    assert(buffer != nullptr);

    vkUnmapMemory(renderer->basics.device, ((VulkanBufferHandle *)buffer)->memory);
}

ImageHandle * VulkanResourceContext::CreateImage(ImageCreateInfo const & ci)
{
    VkImageType const imageType = [](ImageHandle::Type type) {
        switch (type) {
        case ImageHandle::Type::TYPE_1D:
            return VkImageType::VK_IMAGE_TYPE_1D;
        case ImageHandle::Type::TYPE_2D:
            return VkImageType::VK_IMAGE_TYPE_2D;
        default:
            return VkImageType::VK_IMAGE_TYPE_2D;
        }
    }(ci.type);

    VkFormat const format = ToVulkanFormat(ci.format);

    VkExtent3D const extent{ci.width, ci.height, ci.depth};

    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                           nullptr,
                           0,
                           imageType,
                           format,
                           extent,
                           ci.mipLevels,
                           1,
                           VK_SAMPLE_COUNT_1_BIT,
                           VK_IMAGE_TILING_OPTIMAL,
                           (VkImageUsageFlags)ci.usage,
                           VK_SHARING_MODE_EXCLUSIVE,
                           0,
                           nullptr,
                           VK_IMAGE_LAYOUT_UNDEFINED};

    auto const ret = (VulkanImageHandle *)allocator.allocate(sizeof(VulkanImageHandle));
    ret->depth = extent.depth;
    ret->format = ci.format;
    ret->height = extent.height;
    ret->type = ci.type;
    ret->width = extent.width;
    auto const res = vkCreateImage(renderer->basics.device, &info, nullptr, &ret->image);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroyImage(ImageHandle * img)
{
    assert(img != nullptr);

    auto nativeHandle = (VulkanImageHandle *)img;

    vkDestroyImage(renderer->basics.device, nativeHandle->image, nullptr);
    if (nativeHandle->memory != VK_NULL_HANDLE) {
        vkFreeMemory(renderer->basics.device, nativeHandle->memory, nullptr);
    }
    allocator.deallocate((uint8_t *)nativeHandle, sizeof(VulkanImageHandle));
}

void VulkanResourceContext::AllocateImage(ImageHandle * img)
{
    assert(img != nullptr);

    auto const nativeImg = (VulkanImageHandle *)img;

    VkMemoryRequirements const memoryRequirements = [this](VkImage img) {
        VkMemoryRequirements ret{0};
        vkGetImageMemoryRequirements(renderer->basics.device, img, &ret);
        return ret;
    }(nativeImg->image);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(renderer->basics.physicalDevice, &memoryProperties);

    auto memoryIndex = renderer->FindMemoryType(memoryRequirements.memoryTypeBits, 0);

    VkMemoryAllocateInfo const info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memoryRequirements.size, memoryIndex};

    VkDeviceMemory const memory = [this](VkMemoryAllocateInfo const & info) {
        VkDeviceMemory ret{0};
        auto const res = vkAllocateMemory(renderer->basics.device, &info, nullptr, &ret);
        assert(res == VK_SUCCESS);
        return ret;
    }(info);

    nativeImg->memory = memory;
    auto res = vkBindImageMemory(renderer->basics.device, nativeImg->image, memory, 0);
    assert(res == VK_SUCCESS);
}

void VulkanResourceContext::ImageData(ImageHandle * img, std::vector<uint8_t> const & data)
{
    OPTICK_EVENT();
    assert(img != nullptr);

    auto const nativeImg = (VulkanImageHandle *)img;
    VkMemoryRequirements const memoryRequirements = [this](VkImage img) {
        VkMemoryRequirements ret{0};
        vkGetImageMemoryRequirements(renderer->basics.device, img, &ret);
        return ret;
    }(nativeImg->image);

    assert(data.size() <= memoryRequirements.size);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(renderer->basics.physicalDevice, &memoryProperties);

    auto memoryIndex = renderer->FindMemoryType(memoryRequirements.memoryTypeBits, 0);

    VkMemoryAllocateInfo const info{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memoryRequirements.size, memoryIndex};

    VkDeviceMemory const memory = [this](VkMemoryAllocateInfo const & info) {
        VkDeviceMemory ret{0};
        auto const res = vkAllocateMemory(renderer->basics.device, &info, nullptr, &ret);
        assert(res == VK_SUCCESS);
        return ret;
    }(info);

    auto guardedStagingBuffer = renderer->GetStagingBuffer(data.size());
    {
        OPTICK_EVENT("UploadData")
        uint8_t * const mappedMemory = [this](VkDeviceMemory const & memory, size_t size) {
            uint8_t * ret = nullptr;
            auto const res = vkMapMemory(renderer->basics.device, memory, 0, size, 0, (void **)&ret);
            assert(res == VK_SUCCESS);
            return ret;
        }(guardedStagingBuffer.buffer->memory, guardedStagingBuffer.size);

        memcpy(mappedMemory, &data[0], data.size());

        vkUnmapMemory(renderer->basics.device, guardedStagingBuffer.buffer->memory);
    }

    nativeImg->memory = memory;
    VkResult res = vkBindImageMemory(renderer->basics.device, nativeImg->image, memory, 0);
    assert(res == VK_SUCCESS);

    auto cb = renderer->GetStagingCommandBuffer();
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    {
        OPTICK_EVENT("RecordTransferCommands")
        vkBeginCommandBuffer(cb.commandBuffer, &beginInfo);
        renderer->TransitionImageLayout(cb.commandBuffer,
                                        nativeImg->image,
                                        ToVulkanFormat(nativeImg->format),
                                        VK_IMAGE_LAYOUT_UNDEFINED,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        renderer->CopyBufferToImage(cb.commandBuffer,
                                    guardedStagingBuffer.buffer->buffer,
                                    nativeImg->image,
                                    nativeImg->width,
                                    nativeImg->height);
        vkEndCommandBuffer(cb.commandBuffer);
    }

    VkSemaphore sem;
    {
        OPTICK_EVENT("CreateSemaphore")
        VkSemaphoreCreateInfo semInfo = {};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(renderer->basics.device, &semInfo, nullptr, &sem);
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &sem;

    {
        OPTICK_EVENT("SubmitTransferCommands")
        auto transferQueue = renderer->GetTransferQueue();
        vkQueueSubmit(transferQueue.queue, 1, &submitInfo, cb.isReady);
    }

    auto graphicsCommandBuffer = renderer->GetGraphicsStagingCommandBuffer();
    {
        OPTICK_EVENT("RecordGraphicsCommands")
        vkBeginCommandBuffer(graphicsCommandBuffer.commandBuffer, &beginInfo);
        renderer->TransitionImageLayout(graphicsCommandBuffer.commandBuffer,
                                        nativeImg->image,
                                        ToVulkanFormat(nativeImg->format),
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkEndCommandBuffer(graphicsCommandBuffer.commandBuffer);
    }

    VkFence tempFence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(renderer->basics.device, &fenceInfo, nullptr, &tempFence);

    VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo graphicsSubmitInfo = {};
    graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmitInfo.commandBufferCount = 1;
    graphicsSubmitInfo.pCommandBuffers = &graphicsCommandBuffer.commandBuffer;
    graphicsSubmitInfo.waitSemaphoreCount = 1;
    graphicsSubmitInfo.pWaitSemaphores = &sem;
    graphicsSubmitInfo.pWaitDstStageMask = &waitMask;
    {
        OPTICK_EVENT("SubmitGraphicsCommands")
        auto queue = renderer->GetGraphicsQueue();
        vkQueueSubmit(queue.queue, 1, &graphicsSubmitInfo, graphicsCommandBuffer.isReady);
        VkSubmitInfo dummySubmit = {};
        dummySubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        dummySubmit.commandBufferCount = 0;
        vkQueueSubmit(queue.queue, 1, &dummySubmit, tempFence);
    }
    {
        OPTICK_EVENT("FenceWait")
        vkWaitForFences(renderer->basics.device, 1, &tempFence, VK_TRUE, UINT64_MAX);
    }

    vkDestroyFence(renderer->basics.device, tempFence, nullptr);
    vkDestroySemaphore(renderer->basics.device, sem, nullptr);

    assert(res == VK_SUCCESS);
}

RenderPassHandle * VulkanResourceContext::CreateRenderPass(ResourceCreationContext::RenderPassCreateInfo const & ci)
{
    std::vector<VkAttachmentDescription> attachments(ci.attachments.size());
    for (size_t i = 0; i < ci.attachments.size(); ++i) {
        auto description = ci.attachments[i];
        attachments[i] = {description.flags,
                          ToVulkanFormat(description.format),
                          VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT,
                          ToVulkanLoadOp(description.loadOp),
                          ToVulkanStoreOp(description.storeOp),
                          ToVulkanLoadOp(description.stencilLoadOp),
                          ToVulkanStoreOp(description.stencilStoreOp),
                          ToVulkanImageLayout(description.initialLayout),
                          ToVulkanImageLayout(description.finalLayout)};
    }
    std::vector<VkAttachmentReference> colorAttachments;
    std::vector<VkAttachmentReference> depthStencilAttachments;
    std::vector<VkAttachmentReference> inputAttachments;

    for (size_t i = 0; i < ci.subpasses.size(); ++i) {
        auto description = ci.subpasses[i];
        size_t startColorIdx = colorAttachments.size();
        for (uint32_t i = 0; i < description.colorAttachments.size(); ++i) {
            colorAttachments.push_back(ToVulkanAttachmentReference(description.colorAttachments[i]));
        }
        size_t startDepthIdx = depthStencilAttachments.size();
        if (description.depthStencilAttachment.has_value()) {
            depthStencilAttachments.push_back(ToVulkanAttachmentReference(description.depthStencilAttachment.value()));
        }
        size_t startInputIdx = inputAttachments.size();
        for (uint32_t i = 0; i < description.inputAttachments.size(); ++i) {
            inputAttachments.push_back(ToVulkanAttachmentReference(description.inputAttachments[i]));
        }
    }
    std::vector<VkSubpassDescription> subpassDescriptions(ci.subpasses.size());
    size_t currColorIdx = 0;
    size_t currDepthIdx = 0;
    size_t currInputIdx = 0;
    for (size_t i = 0; i < ci.subpasses.size(); ++i) {
        auto description = ci.subpasses[i];
        subpassDescriptions[i] = {
            0,
            ToVulkanPipelineBindPoint(description.pipelineBindPoint),
            (uint32_t)description.inputAttachments.size(),
            description.inputAttachments.size() > 0 ? &inputAttachments[currInputIdx] : nullptr,
            (uint32_t)description.colorAttachments.size(),
            description.colorAttachments.size() > 0 ? &colorAttachments[currColorIdx] : nullptr,
            // TODO
            nullptr,
            description.depthStencilAttachment.has_value() ? &depthStencilAttachments[currDepthIdx] : nullptr,
            (uint32_t)description.preserveAttachments.size(),
            description.preserveAttachments.size() > 0 ? &description.preserveAttachments[0] : nullptr};
        currColorIdx += description.colorAttachments.size();
        currDepthIdx += description.depthStencilAttachment.has_value() ? 1 : 0;
        currInputIdx += description.inputAttachments.size();
    }

    std::vector<VkSubpassDependency> subpassDependencies(ci.subpassDependency.size());
    for (size_t i = 0; i < ci.subpassDependency.size(); ++i) {
        auto dependency = ci.subpassDependency[i];
        subpassDependencies[i] = {dependency.srcSubpass,
                                  dependency.dstSubpass,
                                  dependency.srcStageMask,
                                  dependency.dstStageMask,
                                  dependency.srcAccessMask,
                                  dependency.dstAccessMask,
                                  dependency.dependencyFlags};
    }

    VkRenderPassCreateInfo const info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                      nullptr,
                                      0,
                                      static_cast<uint32_t>(attachments.size()),
                                      attachments.size() > 0 ? &attachments[0] : nullptr,
                                      static_cast<uint32_t>(subpassDescriptions.size()),
                                      subpassDescriptions.size() > 0 ? &subpassDescriptions[0] : nullptr,
                                      static_cast<uint32_t>(subpassDependencies.size()),
                                      subpassDependencies.size() > 0 ? &subpassDependencies[0] : nullptr};

    auto const ret = (VulkanRenderPassHandle *)allocator.allocate(sizeof(VulkanRenderPassHandle));
    auto const res = vkCreateRenderPass(renderer->basics.device, &info, nullptr, &ret->renderPass);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroyRenderPass(RenderPassHandle * pass)
{
    assert(pass != nullptr);

    vkDestroyRenderPass(renderer->basics.device, ((VulkanRenderPassHandle *)pass)->renderPass, nullptr);
    allocator.deallocate((uint8_t *)pass, sizeof(VulkanRenderPassHandle));
}

ImageViewHandle * VulkanResourceContext::CreateImageView(ResourceCreationContext::ImageViewCreateInfo const & ci)
{
    VkImageViewType const type = [](ImageViewHandle::Type type) {
        switch (type) {
        case ImageViewHandle::Type::TYPE_1D:
            return VK_IMAGE_VIEW_TYPE_1D;
        case ImageViewHandle::Type::TYPE_2D:
            return VK_IMAGE_VIEW_TYPE_2D;
        case ImageViewHandle::Type::TYPE_3D:
            return VK_IMAGE_VIEW_TYPE_3D;
        case ImageViewHandle::Type::TYPE_CUBE:
            return VK_IMAGE_VIEW_TYPE_CUBE;
        case ImageViewHandle::Type::TYPE_1D_ARRAY:
            return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case ImageViewHandle::Type::TYPE_2D_ARRAY:
            return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case ImageViewHandle::Type::CUBE_ARRAY:
            return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        default:
            logger.Error("Unknown ImageViewType {}", type);
            assert(false);
            return VK_IMAGE_VIEW_TYPE_1D;
        }
    }(ci.viewType);

    VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                               nullptr,
                               0,
                               ((VulkanImageHandle *)ci.image)->image,
                               type,
                               ToVulkanFormat(ci.format),
                               ToVulkanComponentMapping(ci.components),
                               ToVulkanSubResourceRange(ci.subresourceRange)};

    auto ret = (VulkanImageViewHandle *)allocator.allocate(sizeof(VulkanImageViewHandle));
    auto const res = vkCreateImageView(renderer->basics.device, &info, nullptr, &ret->imageView);
    assert(res == VK_SUCCESS);

    return ret;
}

void VulkanResourceContext::DestroyImageView(ImageViewHandle * view)
{
    assert(view != nullptr);

    vkDestroyImageView(renderer->basics.device, ((VulkanImageViewHandle *)view)->imageView, nullptr);
    allocator.deallocate((uint8_t *)view, sizeof(VulkanImageViewHandle));
}

FramebufferHandle * VulkanResourceContext::CreateFramebuffer(ResourceCreationContext::FramebufferCreateInfo const & ci)
{
    std::vector<VkImageView> imageViews(ci.attachments.size());
    for (size_t i = 0; i < ci.attachments.size(); ++i) {
        imageViews[i] = ((VulkanImageViewHandle *)ci.attachments[i])->imageView;
    }
    VkFramebufferCreateInfo const info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                       nullptr,
                                       0,
                                       ((VulkanRenderPassHandle *)ci.renderPass)->renderPass,
                                       static_cast<uint32_t>(imageViews.size()),
                                       &imageViews[0],
                                       ci.width,
                                       ci.height,
                                       ci.layers};

    auto ret = (VulkanFramebufferHandle *)allocator.allocate(sizeof(VulkanFramebufferHandle));
    auto const res = vkCreateFramebuffer(renderer->basics.device, &info, nullptr, &ret->framebuffer);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroyFramebuffer(FramebufferHandle * fb)
{
    assert(fb != nullptr);

    vkDestroyFramebuffer(renderer->basics.device, ((VulkanFramebufferHandle *)fb)->framebuffer, nullptr);
    allocator.deallocate((uint8_t *)fb, sizeof(VulkanFramebufferHandle));
}

PipelineLayoutHandle * VulkanResourceContext::CreatePipelineLayout(PipelineLayoutCreateInfo const & ci)
{
    std::vector<VkDescriptorSetLayout> nativeLayouts(ci.setLayouts.size());
    for (size_t i = 0; i < ci.setLayouts.size(); ++i) {
        nativeLayouts[i] = ((VulkanDescriptorSetLayoutHandle *)ci.setLayouts[i])->layout;
    }

    VkPipelineLayoutCreateInfo layoutInfo;
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.setLayoutCount = nativeLayouts.size();
    layoutInfo.pSetLayouts = &nativeLayouts[0];
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;

    auto ret = (VulkanPipelineLayoutHandle *)allocator.allocate(sizeof(VulkanPipelineLayoutHandle));
    auto res = vkCreatePipelineLayout(renderer->basics.device, &layoutInfo, nullptr, &ret->pipelineLayout);
    assert(res == VK_SUCCESS);

    return ret;
}

void VulkanResourceContext::DestroyPipelineLayout(PipelineLayoutHandle * layout)
{
    assert(layout != nullptr);

    vkDestroyPipelineLayout(renderer->basics.device, ((VulkanPipelineLayoutHandle *)layout)->pipelineLayout, nullptr);
    allocator.deallocate((uint8_t *)layout, sizeof(VulkanPipelineLayoutHandle));
}

PipelineHandle *
VulkanResourceContext::CreateGraphicsPipeline(ResourceCreationContext::GraphicsPipelineCreateInfo const & ci)
{
    assert(ci.rasterizationState != nullptr);

    std::vector<uint32_t> specializationData;
    specializationData.reserve(ci.specializationConstants.size() + 1);
    std::vector<VkSpecializationMapEntry> specializationMapEntries;
    specializationMapEntries.reserve(ci.specializationConstants.size() + 1);

    // gfxApi specialization constant. 0 == Vulkan
    specializationData.push_back(SpecializationConstants::GFX_API_VULKAN_VALUE);
    specializationMapEntries.push_back(VkSpecializationMapEntry{
        .constantID = SpecializationConstants::GFX_API_CONSTANT_ID, .offset = 0, .size = sizeof(uint32_t)});

    for (auto const & kv : ci.specializationConstants) {
        if (kv.first == 0) {
            logger.Error("Attempted to set specialization constant with key=0, this key is reserved for the gfxApi "
                         "specialization constant and will be ignored.");
            continue;
        }
        specializationData.push_back(kv.second);
        specializationMapEntries.push_back(
            VkSpecializationMapEntry{.constantID = kv.first,
                                     .offset = (uint32_t)((specializationData.size() - 1) * sizeof(uint32_t)),
                                     .size = sizeof(uint32_t)});
    }

    VkSpecializationInfo specializationInfo = {};
    specializationInfo.mapEntryCount = (uint32_t)specializationMapEntries.size();
    specializationInfo.pMapEntries = &specializationMapEntries[0];
    specializationInfo.dataSize = specializationData.size() * sizeof(uint32_t);
    specializationInfo.pData = &specializationData[0];

    std::vector<VkPipelineShaderStageCreateInfo> stages(ci.stageCount);

    std::transform(ci.pStages,
                   &ci.pStages[ci.stageCount],
                   stages.begin(),
                   [&](ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo stageInfo) {
                       auto nativeShader = (VulkanShaderModuleHandle *)stageInfo.module;
                       return VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                              nullptr,
                                                              0,
                                                              ToVulkanShaderStage(stageInfo.stage),
                                                              nativeShader->shader,
                                                              "main",
                                                              &specializationInfo};
                   });

    auto nativeVertexInputState = ((VulkanVertexInputStateHandle *)ci.vertexInputState)->createInfo;
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(
        nativeVertexInputState.vertexBindingDescriptions.size());
    for (size_t i = 0; i < bindingDescriptions.size(); ++i) {
        auto & description = nativeVertexInputState.vertexBindingDescriptions[i];
        bindingDescriptions[i] =
            VkVertexInputBindingDescription{description.binding, description.stride, VK_VERTEX_INPUT_RATE_VERTEX};
    }

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(
        nativeVertexInputState.vertexAttributeDescriptions.size());
    for (size_t i = 0; i < attributeDescriptions.size(); ++i) {
        auto & description = nativeVertexInputState.vertexAttributeDescriptions[i];
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        if (description.type == VertexComponentType::FLOAT) {
            if (description.size == 1) {
                format = VK_FORMAT_R32_SFLOAT;
            } else if (description.size == 2) {
                format = VK_FORMAT_R32G32_SFLOAT;
            } else if (description.size == 3) {
                format = VK_FORMAT_R32G32B32_SFLOAT;
            } else if (description.size == 4) {
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
            }
        } else if (description.type == VertexComponentType::UINT) {
            if (description.size == 1) {
                format = VK_FORMAT_R32_UINT;
            } else if (description.size == 2) {
                format = VK_FORMAT_R32G32_UINT;
            } else if (description.size == 3) {
                format = VK_FORMAT_R32G32B32_UINT;
            } else if (description.size == 4) {
                format = VK_FORMAT_R32G32B32A32_UINT;
            }
        }
        attributeDescriptions[i] =
            VkVertexInputAttributeDescription{description.location, description.binding, format, description.offset};
    }

    VkPipelineVertexInputStateCreateInfo vertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                                          nullptr,
                                                          0,
                                                          (uint32_t)bindingDescriptions.size(),
                                                          &bindingDescriptions[0],
                                                          (uint32_t)attributeDescriptions.size(),
                                                          &attributeDescriptions[0]};

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    if (ci.inputAssembly.topology == PrimitiveTopology::POINT_LIST) {
        topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    } else if (ci.inputAssembly.topology == PrimitiveTopology::LINE_LIST) {
        topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0, topology, VK_FALSE};

    VkRect2D scissor{{0, 0}, renderer->swapchain.extent};
    VkViewport viewport{
        0.f, 0.f, (float)renderer->swapchain.extent.width, (float)renderer->swapchain.extent.height, 0.f, 1.f};
    VkPipelineViewportStateCreateInfo viewportState{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0, 1, &viewport, 1, &scissor};

    VkPipelineRasterizationStateCreateInfo raserizationState{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                                             nullptr,
                                                             0,
                                                             VK_FALSE,
                                                             VK_FALSE,
                                                             VK_POLYGON_MODE_FILL,
                                                             ToVulkanCullMode(ci.rasterizationState->cullMode),
                                                             ToVulkanFrontFace(ci.rasterizationState->frontFace),
                                                             VK_FALSE,
                                                             0.f,
                                                             0.f,
                                                             0.f,
                                                             4.f};

    VkPipelineMultisampleStateCreateInfo multisampleState{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                                          nullptr,
                                                          0,
                                                          VK_SAMPLE_COUNT_1_BIT,
                                                          VK_FALSE,
                                                          1.f,
                                                          nullptr,
                                                          VK_FALSE,
                                                          VK_FALSE};

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(ci.colorBlendAttachments.size());
    for (size_t i = 0; i < ci.colorBlendAttachments.size(); ++i) {
        if (ci.colorBlendAttachments[i].blendMode ==
            GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_ENABLED) {
            colorBlendAttachments[i] = {.blendEnable = VK_TRUE,
                                        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                                        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                        .colorBlendOp = VK_BLEND_OP_ADD,
                                        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                        .alphaBlendOp = VK_BLEND_OP_ADD,
                                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
        } else if (ci.colorBlendAttachments[i].blendMode ==
                   GraphicsPipelineCreateInfo::AttachmentBlendMode::BLENDING_DISABLED) {
            colorBlendAttachments[i] = {.blendEnable = VK_FALSE,
                                        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                                        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                        .colorBlendOp = VK_BLEND_OP_ADD,
                                        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                        .alphaBlendOp = VK_BLEND_OP_ADD,
                                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
        } else if (ci.colorBlendAttachments[i].blendMode ==
                   GraphicsPipelineCreateInfo::AttachmentBlendMode::ATTACHMENT_DISABLED) {
            colorBlendAttachments[i] = {.blendEnable = VK_FALSE,
                                        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                                        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                                        .colorBlendOp = VK_BLEND_OP_ADD,
                                        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                        .alphaBlendOp = VK_BLEND_OP_ADD,
                                        .colorWriteMask = 0};
        } else {
            assert(false);
        }
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                        nullptr,
                                                        0,
                                                        VK_FALSE,
                                                        VK_LOGIC_OP_COPY,
                                                        static_cast<uint32_t>(colorBlendAttachments.size()),
                                                        colorBlendAttachments.size() > 0 ? &colorBlendAttachments[0]
                                                                                         : nullptr,
                                                        {0.f, 0.f, 0.f, 0.f}};

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                            nullptr,
                                                            0,
                                                            sizeof(dynamicStates) / sizeof(VkDynamicState),
                                                            dynamicStates};

    VkPipelineDepthStencilStateCreateInfo depthStateCreateInfo;
    depthStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStateCreateInfo.pNext = nullptr;
    depthStateCreateInfo.depthTestEnable = ci.depthStencil->depthTestEnable;
    depthStateCreateInfo.depthWriteEnable = ci.depthStencil->depthWriteEnable;
    depthStateCreateInfo.depthCompareOp = ToVulkanCompareOp(ci.depthStencil->depthCompareOp);

    depthStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStateCreateInfo.minDepthBounds = 0.f;
    depthStateCreateInfo.maxDepthBounds = 1.f;
    depthStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStateCreateInfo.front = {};
    depthStateCreateInfo.back = {};
    depthStateCreateInfo.flags = 0;

    auto layout = ((VulkanPipelineLayoutHandle *)ci.pipelineLayout)->pipelineLayout;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = ci.stageCount;
    pipelineInfo.pStages = &stages[0];
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &raserizationState;
    pipelineInfo.pMultisampleState = &multisampleState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineInfo.pDepthStencilState = &depthStateCreateInfo;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = ((VulkanRenderPassHandle *)ci.renderPass)->renderPass;
    pipelineInfo.subpass = ci.subpass;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    auto ret = (VulkanPipelineHandle *)allocator.allocate(sizeof(VulkanPipelineHandle));
    auto res =
        vkCreateGraphicsPipelines(renderer->basics.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ret->pipeline);
    assert(res == VK_SUCCESS);
    ret->vertexInputState = ci.vertexInputState;

    return ret;
}

void VulkanResourceContext::DestroyPipeline(PipelineHandle * pipeline)
{
    assert(pipeline != nullptr);

    vkDestroyPipeline(renderer->basics.device, ((VulkanPipelineHandle *)pipeline)->pipeline, nullptr);
    allocator.deallocate((uint8_t *)pipeline, sizeof(VulkanPipelineHandle));
}

ShaderModuleHandle *
VulkanResourceContext::CreateShaderModule(ResourceCreationContext::ShaderModuleCreateInfo const & ci)
{
    VkShaderModuleCreateInfo info{
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, ci.code.size() * 4, ci.code.data()};

    auto ret = (VulkanShaderModuleHandle *)allocator.allocate(sizeof(ShaderModuleHandle *));
    auto const res = vkCreateShaderModule(renderer->basics.device, &info, nullptr, &ret->shader);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroyShaderModule(ShaderModuleHandle * shader)
{
    assert(shader != nullptr);

    vkDestroyShaderModule(renderer->basics.device, ((VulkanShaderModuleHandle *)shader)->shader, nullptr);
    allocator.deallocate((uint8_t *)shader, sizeof(VulkanShaderModuleHandle));
}

SamplerHandle * VulkanResourceContext::CreateSampler(ResourceCreationContext::SamplerCreateInfo const & ci)
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = ToVulkanFilter(ci.magFilter);
    samplerInfo.minFilter = ToVulkanFilter(ci.minFilter);
    samplerInfo.addressModeU = ToVulkanAddressMode(ci.addressModeU);
    samplerInfo.addressModeV = ToVulkanAddressMode(ci.addressModeV);
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    auto ret = (VulkanSamplerHandle *)allocator.allocate(sizeof(VulkanSamplerHandle));
    auto res = vkCreateSampler(renderer->basics.device, &samplerInfo, nullptr, &ret->sampler);
    assert(res == VK_SUCCESS);

    return ret;
}

void VulkanResourceContext::DestroySampler(SamplerHandle * sampler)
{
    assert(sampler != nullptr);
    vkDestroySampler(renderer->basics.device, ((VulkanSamplerHandle *)sampler)->sampler, nullptr);
    allocator.deallocate((uint8_t *)sampler, sizeof(VulkanSamplerHandle));
}

SemaphoreHandle * VulkanResourceContext::CreateSemaphore()
{
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    auto ret = (VulkanSemaphoreHandle *)allocator.allocate(sizeof(VulkanSemaphoreHandle));
    auto res = vkCreateSemaphore(renderer->basics.device, &createInfo, nullptr, &ret->semaphore);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroySemaphore(SemaphoreHandle * sem)
{
    assert(sem != nullptr);
    vkDestroySemaphore(renderer->basics.device, ((VulkanSemaphoreHandle *)sem)->semaphore, nullptr);
    allocator.deallocate((uint8_t *)sem, sizeof(VulkanSemaphoreHandle));
}

DescriptorSetLayoutHandle * VulkanResourceContext::CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo const & info)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(info.bindingCount);
    for (uint32_t i = 0; i < info.bindingCount; ++i) {
        bindings.push_back(VkDescriptorSetLayoutBinding{info.pBinding[i].binding,
                                                        ToVulkanDescriptorType(info.pBinding[i].descriptorType),
                                                        1,
                                                        info.pBinding[i].stageFlags,
                                                        nullptr});
    }
    VkDescriptorSetLayoutCreateInfo ci{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, info.bindingCount, &bindings[0]};
    auto ret = (VulkanDescriptorSetLayoutHandle *)allocator.allocate(sizeof(VulkanDescriptorSetLayoutHandle));
    auto res = vkCreateDescriptorSetLayout(renderer->basics.device, &ci, nullptr, &(ret->layout));
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle * layout)
{
    assert(layout != nullptr);
    vkDestroyDescriptorSetLayout(renderer->basics.device, ((VulkanDescriptorSetLayoutHandle *)layout)->layout, nullptr);
    allocator.deallocate((uint8_t *)layout, sizeof(VulkanDescriptorSetLayoutHandle));
}

FenceHandle * VulkanResourceContext::CreateFence(bool startSignaled)
{
    VkFenceCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    ci.flags = startSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

    auto mem = allocator.allocate(sizeof(VulkanFenceHandle));
    auto ret = new (mem) VulkanFenceHandle();
    ret->device = renderer->basics.device;
    auto res = vkCreateFence(renderer->basics.device, &ci, nullptr, &ret->fence);
    assert(res == VK_SUCCESS);
    return ret;
}

void VulkanResourceContext::DestroyFence(FenceHandle * fence)
{
    assert(fence != nullptr);
    auto nativeHandle = (VulkanFenceHandle *)fence;
    vkDestroyFence(renderer->basics.device, nativeHandle->fence, nullptr);
}

VertexInputStateHandle *
VulkanResourceContext::CreateVertexInputState(ResourceCreationContext::VertexInputStateCreateInfo & info)
{
    auto ret = (VulkanVertexInputStateHandle *)allocator.allocate(sizeof(VulkanVertexInputStateHandle));
    ret = new (ret) VulkanVertexInputStateHandle();
    ret->createInfo = info;
    return ret;
}

void VulkanResourceContext::DestroyVertexInputState(VertexInputStateHandle * state)
{
    assert(state != nullptr);
    allocator.deallocate((uint8_t *)state, sizeof(VulkanVertexInputStateHandle));
}

DescriptorSet * VulkanResourceContext::CreateDescriptorSet(DescriptorSetCreateInfo const & info)
{
    OPTICK_EVENT();
    assert(info.layout != nullptr);
    VkDescriptorSetLayout layout = ((VulkanDescriptorSetLayoutHandle *)info.layout)->layout;

    auto threadIdx = JobEngine::GetInstance()->GetCurrentThreadIndex();

    VkDescriptorSetAllocateInfo ai{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, renderer->descriptorPools[threadIdx].pool, 1, &layout};

    VkDescriptorSet set;
    {
        std::lock_guard<std::mutex> lock(renderer->descriptorPools[threadIdx].poolLock);
        auto res = vkAllocateDescriptorSets(renderer->basics.device, &ai, &set);
        assert(res == VK_SUCCESS);
    }

    std::unordered_map<uint32_t, size_t> bindingToIdx;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    for (size_t i = 0; i < info.descriptorCount; ++i) {
        auto & descriptor = info.descriptors[i];
        if (IsBufferDescriptorType(descriptor.type)) {
            auto buffer =
                std::get<ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor>(descriptor.descriptor);
            auto nativeBuffer = (VulkanBufferHandle *)buffer.buffer;
            bufferInfos.push_back({nativeBuffer->buffer, buffer.offset, buffer.range});
            bindingToIdx.insert_or_assign(descriptor.binding, bufferInfos.size() - 1);
        } else if (descriptor.type == DescriptorType::COMBINED_IMAGE_SAMPLER) {
            auto image =
                std::get<ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor>(descriptor.descriptor);
            imageInfos.push_back({((VulkanSamplerHandle *)image.sampler)->sampler,
                                  ((VulkanImageViewHandle *)image.imageView)->imageView,
                                  // TODO: Add this to the createInfo?
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            bindingToIdx.insert_or_assign(descriptor.binding, imageInfos.size() - 1);
        } else if (descriptor.type == DescriptorType::INPUT_ATTACHMENT) {
            auto image =
                std::get<ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor>(descriptor.descriptor);
            imageInfos.push_back({VK_NULL_HANDLE,
                                  ((VulkanImageViewHandle *)image.imageView)->imageView,
                                  // TODO: Add this to the createInfo?
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            bindingToIdx.insert_or_assign(descriptor.binding, imageInfos.size() - 1);
        } else {
            logger.Error(
                "Unknown descriptorType={} at binding={}, will likely crash", descriptor.type, descriptor.binding);
        }
    }

    std::vector<VkWriteDescriptorSet> writes(info.descriptorCount);
    for (size_t i = 0; i < info.descriptorCount; ++i) {
        auto & descriptor = info.descriptors[i];
        auto idx = bindingToIdx.at(descriptor.binding);
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     nullptr,
                     set,
                     descriptor.binding,
                     0,
                     1,
                     ToVulkanDescriptorType(descriptor.type),
                     IsImageDescriptorType(descriptor.type) ? &imageInfos[idx] : nullptr,
                     IsBufferDescriptorType(descriptor.type) ? &bufferInfos[idx] : nullptr,
                     // TODO: if UNIFORM_TEXEL_BUFFER/STORAGE_TEXEL_BUFFER uniforms become available
                     nullptr};
    }

    vkUpdateDescriptorSets(renderer->basics.device, (uint32_t)writes.size(), &writes[0], 0, nullptr);

    auto ret = (VulkanDescriptorSet *)allocator.allocate(sizeof(VulkanDescriptorSet));
    ret->layout = layout;
    ret->set = set;
    ret->poolIdx = threadIdx;

    return ret;
}

void VulkanResourceContext::DestroyDescriptorSet(DescriptorSet * set)
{
    assert(set != nullptr);

    auto nativeDescriptorSet = (VulkanDescriptorSet *)set;

    auto threadIdx = JobEngine::GetInstance()->GetCurrentThreadIndex();

    {
        std::lock_guard<std::mutex> lock(renderer->descriptorPools[nativeDescriptorSet->poolIdx].poolLock);
        auto res = vkFreeDescriptorSets(renderer->basics.device,
                                        renderer->descriptorPools[nativeDescriptorSet->poolIdx].pool,
                                        1,
                                        &(nativeDescriptorSet->set));
        assert(res == VK_SUCCESS);
    }
    allocator.deallocate((uint8_t *)nativeDescriptorSet, sizeof(VulkanDescriptorSet));
}

bool VulkanFenceHandle::Wait(uint64_t timeOut)
{
    auto res = vkWaitForFences(device, 1, &fence, VK_TRUE, timeOut);
    vkResetFences(device, 1, &fence);
    return res != VK_TIMEOUT;
}

#endif
