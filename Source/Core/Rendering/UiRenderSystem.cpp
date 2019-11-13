#include "Core/Rendering/UiRenderSystem.h"

#include <imgui.h>

#include "Core/ResourceManager.h"
#include "Core/Semaphore.h"
#include "Core/dtime.h"

UiRenderSystem::UiRenderSystem(Renderer * renderer, ResourceManager * resourceManager)
    : renderer(renderer), resourceManager(resourceManager), frameData(renderer->GetSwapCount())
{
    layout = resourceManager->GetResource<DescriptorSetLayoutHandle>(
        "_Primitives/DescriptorSetLayouts/ui.layout");
    pipelineLayout = resourceManager->GetResource<PipelineLayoutHandle>(
        "_Primitives/PipelineLayouts/ui.pipelinelayout");
    gfxPipeline = resourceManager->GetResource<PipelineHandle>("_Primitives/Pipelines/ui.pipe");

    auto & imguiIo = ImGui::GetIO();
    imguiIo.MouseDrawCursor = false;
    auto res = renderer->GetResolution();
    imguiIo.DisplaySize = ImVec2(res.x, res.y);
    uint8_t * fontPixels;
    int fontWidth, fontHeight, bytesPerPixel;
    imguiIo.Fonts->AddFontDefault();
    imguiIo.Fonts->GetTexDataAsRGBA32(&fontPixels, &fontWidth, &fontHeight, &bytesPerPixel);
    std::vector<uint8_t> fontPixelVector(fontWidth * fontHeight * bytesPerPixel);
    memcpy(&fontPixelVector[0], fontPixels, fontWidth * fontHeight * bytesPerPixel);

    Semaphore sem;
    renderer->CreateResources([&](ResourceCreationContext & ctx) {
        ResourceCreationContext::ImageCreateInfo fontCreateInfo;
        fontCreateInfo.depth = 1;
        fontCreateInfo.format = Format::RGBA8;
        fontCreateInfo.height = fontHeight;
        fontCreateInfo.width = fontWidth;
        fontCreateInfo.mipLevels = 1;
        fontCreateInfo.type = ImageHandle::Type::TYPE_2D;
        fontCreateInfo.usage = ImageUsageFlagBits::IMAGE_USAGE_FLAG_SAMPLED_BIT |
                               ImageUsageFlagBits::IMAGE_USAGE_FLAG_TRANSFER_DST_BIT;
        fontAtlas = ctx.CreateImage(fontCreateInfo);
        ctx.ImageData(fontAtlas, fontPixelVector);
        imguiIo.Fonts->TexID = fontAtlas;

        {
            ResourceCreationContext::ImageViewCreateInfo ci;
            ci.components = ImageViewHandle::ComponentMapping::IDENTITY;
            ci.format = Format::RGBA8;
            ci.image = fontAtlas;
            ci.subresourceRange.aspectMask = ImageViewHandle::ImageAspectFlagBits::COLOR_BIT;
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.baseMipLevel = 0;
            ci.subresourceRange.layerCount = 1;
            ci.subresourceRange.levelCount = 1;
            ci.viewType = ImageViewHandle::Type::TYPE_2D;
            fontAtlasView = ctx.CreateImageView(ci);
        }

        {
            ResourceCreationContext::SamplerCreateInfo ci;
            ci.addressModeU = AddressMode::REPEAT;
            ci.addressModeV = AddressMode::REPEAT;
            ci.magFilter = Filter::LINEAR;
            ci.minFilter = Filter::LINEAR;
            fontSampler = ctx.CreateSampler(ci);
        }

        {
            ResourceCreationContext::DescriptorSetCreateInfo ci;
            ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imageDescriptor;
            imageDescriptor.imageView = fontAtlasView;
            imageDescriptor.sampler = fontSampler;
            ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptor;
            descriptor.binding = 0;
            descriptor.descriptor = imageDescriptor;
            descriptor.type = DescriptorType::COMBINED_IMAGE_SAMPLER;
            ci.descriptorCount = 1;
            ci.descriptors = &descriptor;
            ci.layout = layout;
            descriptorSet = ctx.CreateDescriptorSet(ci);
            sem.Signal();
        }
        sem.Wait();
    });
}

void UiRenderSystem::StartFrame()
{
    auto deltaTime = Time::GetDeltaTime();
    auto & io = ImGui::GetIO();
    io.DeltaTime = Time::GetUnscaledDeltaTime();
    ImGui::NewFrame();
}

void UiRenderSystem::PreRenderUi(uint32_t frameIndex, CommandBuffer * commandBuffer)
{
    ImGui::Render();
    auto data = ImGui::GetDrawData();
    auto & fd = frameData[frameIndex];
    size_t totalIndexSize = data->TotalIdxCount * sizeof(ImDrawIdx);
    size_t totalVertexSize = data->TotalVtxCount * sizeof(ImDrawVert);
    if (totalIndexSize == 0 || totalVertexSize == 0) {
        return;
    }
    Semaphore resourceCreationFinished;
    renderer->CreateResources([&](ResourceCreationContext & ctx) {
        if (fd.vertexBuffer == nullptr || fd.vertexBufferSize < totalVertexSize) {
            if (fd.vertexBuffer != nullptr) {
                ctx.DestroyBuffer(fd.vertexBuffer);
            }
            fd.vertexBufferSize = totalVertexSize;
            ResourceCreationContext::BufferCreateInfo ci;
            ci.memoryProperties = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT;
            ci.size = totalVertexSize;
            ci.usage = BufferUsageFlags::VERTEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT;
            fd.vertexBuffer = ctx.CreateBuffer(ci);
        }
        if (fd.indexBuffer == nullptr || fd.indexBufferSize < totalIndexSize) {
            if (fd.indexBuffer != nullptr) {
                ctx.DestroyBuffer(fd.indexBuffer);
            }
            fd.indexBufferSize = totalIndexSize;
            ResourceCreationContext::BufferCreateInfo ci;
            ci.memoryProperties = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT;
            ci.size = totalIndexSize;
            ci.usage = BufferUsageFlags::INDEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT;
            fd.indexBuffer = ctx.CreateBuffer(ci);
        }
        size_t currIndexPos = 0;
        size_t currVertexPos = 0;
        for (int i = 0; i < data->CmdListsCount; ++i) {
            auto cmdList = data->CmdLists[i];
            ctx.BufferSubData(fd.indexBuffer, (uint8_t *)cmdList->IdxBuffer.Data, currIndexPos,
                              cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            ctx.BufferSubData(fd.vertexBuffer, (uint8_t *)cmdList->VtxBuffer.Data, currVertexPos,
                              cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            currIndexPos += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
            currVertexPos += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        }
        resourceCreationFinished.Signal();
    });
    resourceCreationFinished.Wait();
}

void UiRenderSystem::RenderUi(uint32_t frameIndex, CommandBuffer * commandBuffer)
{
    auto data = ImGui::GetDrawData();
    auto & fd = frameData[frameIndex];

    if (!fd.indexBuffer || !fd.vertexBuffer) {
        return;
    }

    commandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS, gfxPipeline);
    commandBuffer->CmdBindDescriptorSets(pipelineLayout, 0, {descriptorSet});
    commandBuffer->CmdBindIndexBuffer(fd.indexBuffer, 0, CommandBuffer::IndexType::UINT16);
    commandBuffer->CmdBindVertexBuffer(fd.vertexBuffer, 0, 0, sizeof(ImDrawVert));
    // TODO: translation
    size_t currIndexPos = 0;
    size_t currVertexPos = 0;
    for (int i = 0; i < data->CmdListsCount; ++i) {
        auto cmdList = data->CmdLists[i];
        for (int j = 0; j < cmdList->CmdBuffer.Size; ++j) {
            auto imCmdBuf = &cmdList->CmdBuffer[j];
            if (imCmdBuf->UserCallback != nullptr) {
                imCmdBuf->UserCallback(cmdList, imCmdBuf);
            } else {
                CommandBuffer::Rect2D scissor;
                scissor.offset.x = imCmdBuf->ClipRect.x > 0 ? imCmdBuf->ClipRect.x : 0;
                scissor.offset.y = imCmdBuf->ClipRect.y > 0 ? imCmdBuf->ClipRect.y : 0;
                scissor.extent.width = imCmdBuf->ClipRect.z - imCmdBuf->ClipRect.x;
                scissor.extent.height = imCmdBuf->ClipRect.w - imCmdBuf->ClipRect.y; //+1;
                commandBuffer->CmdSetScissor(0, 1, &scissor);
                commandBuffer->CmdDrawIndexed(imCmdBuf->ElemCount, 1, currIndexPos, currVertexPos);
            }
            currIndexPos += imCmdBuf->ElemCount;
        }
        currVertexPos += cmdList->VtxBuffer.Size;
    }
}
