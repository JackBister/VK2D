#include "Core/Rendering/UiRenderSystem.h"

#include <imgui.h>

#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"
#include "Core/Semaphore.h"
#include "Core/dtime.h"

UiRenderSystem::UiRenderSystem(Renderer * renderer) : renderer(renderer), frameData(renderer->GetSwapCount())
{
    auto & imguiIo = ImGui::GetIO();
    imguiIo.MouseDrawCursor = false;
    auto res = renderer->GetResolution();
    imguiIo.DisplaySize = ImVec2(res.x, res.y);
}

void UiRenderSystem::Init()
{
    layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/ui.layout");
    pipelineLayout =
        ResourceManager::GetResource<PipelineLayoutHandle>("_Primitives/PipelineLayouts/ui.pipelinelayout");
    uiProgram = ResourceManager::GetResource<ShaderProgram>("_Primitives/ShaderPrograms/ui.program");

    ResourceManager::CreateResources([this](ResourceCreationContext & ctx) {
        ResourceCreationContext::BufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.memoryProperties = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT;
        bufferCreateInfo.size = sizeof(glm::vec2);
        bufferCreateInfo.usage = BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT;
        this->resolutionUniformBuffer = ctx.CreateBuffer(bufferCreateInfo);

        auto fontAtlasView =
            ResourceManager::GetResource<ImageViewHandle>("_Primitives/Images/FontAtlas.img/defaultView");
        auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");

        ResourceCreationContext::DescriptorSetCreateInfo descriptorSetCreateInfo;

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor bufferDescriptor;
        bufferDescriptor.buffer = this->resolutionUniformBuffer;
        bufferDescriptor.offset = 0;
        bufferDescriptor.range = sizeof(glm::vec2);

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imageDescriptor;
        imageDescriptor.imageView = fontAtlasView;
        imageDescriptor.sampler = sampler;
        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, bufferDescriptor},
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 1, imageDescriptor}};
        descriptorSetCreateInfo.descriptorCount = 2;
        descriptorSetCreateInfo.descriptors = descriptors;
        descriptorSetCreateInfo.layout = layout;
        this->descriptorSet = ctx.CreateDescriptorSet(descriptorSetCreateInfo);
        ResourceManager::AddResource("_Primitives/DescriptorSets/ui.descriptorSet", this->descriptorSet);
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
    glm::vec2 res = renderer->GetResolution();
    auto & imguiIo = ImGui::GetIO();
    imguiIo.DisplaySize = ImVec2(res.x, res.y);
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
            ctx.BufferSubData(fd.indexBuffer,
                              (uint8_t *)cmdList->IdxBuffer.Data,
                              currIndexPos,
                              cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            ctx.BufferSubData(fd.vertexBuffer,
                              (uint8_t *)cmdList->VtxBuffer.Data,
                              currVertexPos,
                              cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            currIndexPos += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
            currVertexPos += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        }
        resourceCreationFinished.Signal();
    });
    resourceCreationFinished.Wait();

    commandBuffer->CmdUpdateBuffer(resolutionUniformBuffer, 0, sizeof(glm::vec2), (uint32_t *)&res);
}

void UiRenderSystem::RenderUi(uint32_t frameIndex, CommandBuffer * commandBuffer)
{
    auto data = ImGui::GetDrawData();
    auto & fd = frameData[frameIndex];

    if (!fd.indexBuffer || !fd.vertexBuffer) {
        return;
    }

    commandBuffer->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS, uiProgram->GetPipeline());
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
                auto res = renderer->GetResolution();
                CommandBuffer::Viewport viewport = {0.f, 0.f, res.x, res.y, 0.f, 1.f};
                commandBuffer->CmdSetViewport(0, 1, &viewport);

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
