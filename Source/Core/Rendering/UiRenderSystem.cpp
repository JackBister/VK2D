#include "Core/Rendering/UiRenderSystem.h"

#include <imgui.h>

#include <ImGuizmo.h>
#include <optick/optick.h>

#include "Core/FrameContext.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Resources/ShaderProgram.h"
#include "Core/dtime.h"
#include "Util/Semaphore.h"

UiRenderSystem * UiRenderSystem::instance = nullptr;

UiRenderSystem * UiRenderSystem::GetInstance()
{
    return UiRenderSystem::instance;
}

ImDrawData * CopyDrawData(ImDrawData * original)
{
    auto ret = new ImDrawData(*original);
    ret->CmdLists = (ImDrawList **)malloc(ret->CmdListsCount * sizeof(ImDrawList *));
    for (int i = 0; i < ret->CmdListsCount; ++i) {
        ret->CmdLists[i] = original->CmdLists[i]->CloneOutput();
    }
    return ret;
}

UiRenderSystem::UiRenderSystem(Renderer * renderer) : renderer(renderer), frameData(renderer->GetSwapCount())
{
    auto & imguiIo = ImGui::GetIO();
    imguiIo.MouseDrawCursor = false;
    auto res = renderer->GetResolution();
    imguiIo.DisplaySize = ImVec2(res.x, res.y);
    UiRenderSystem::instance = this;
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
    OPTICK_EVENT();
    auto deltaTime = Time::GetDeltaTime();
    auto & io = ImGui::GetIO();
    io.DeltaTime = Time::GetUnscaledDeltaTime();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
    glm::vec2 res = renderer->GetResolution();
    ImGuizmo::SetRect(0, 0, res.x, res.y);
}

void UiRenderSystem::EndFrame(FrameContext & context)
{
    OPTICK_EVENT();
    glm::vec2 res = renderer->GetResolution();
    auto & imguiIo = ImGui::GetIO();
    imguiIo.DisplaySize = ImVec2(res.x, res.y);
    ImGui::Render();
    context.imguiDrawData = CopyDrawData(ImGui::GetDrawData());
}

void UiRenderSystem::PreRenderUi(FrameContext const & context, CommandBuffer * commandBuffer)
{
    OPTICK_EVENT();
    auto data = context.imguiDrawData;
    auto & fd = frameData[context.currentGpuFrameIndex];
    size_t totalIndexSize = data->TotalIdxCount * sizeof(ImDrawIdx);
    size_t totalVertexSize = data->TotalVtxCount * sizeof(ImDrawVert);
    if (totalIndexSize == 0 || totalVertexSize == 0) {
        return;
    }
    if (fd.vertexBuffer == nullptr || fd.vertexBufferSize < totalVertexSize || fd.indexBuffer == nullptr ||
        fd.indexBufferSize < totalIndexSize) {
        RecreateBuffers(context.currentGpuFrameIndex, totalVertexSize, totalIndexSize);
    }

    size_t currIndexPos = 0;
    size_t currVertexPos = 0;
    for (int i = 0; i < data->CmdListsCount; ++i) {
        auto cmdList = data->CmdLists[i];
        memcpy(
            fd.indexBufferMapped + currIndexPos, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        memcpy(fd.vertexBufferMapped + currVertexPos,
               cmdList->VtxBuffer.Data,
               cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        currIndexPos += cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        currVertexPos += cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
    }

    glm::vec2 res = renderer->GetResolution();
    commandBuffer->CmdUpdateBuffer(resolutionUniformBuffer, 0, sizeof(glm::vec2), (uint32_t *)&res);
}

void UiRenderSystem::RecreateBuffers(uint32_t frameIndex, size_t totalVertexSize, size_t totalIndexSize)
{
    auto & fd = frameData[frameIndex];
    Semaphore resourceCreationFinished;
    renderer->CreateResources([&](ResourceCreationContext & ctx) {
        if (fd.vertexBuffer == nullptr || fd.vertexBufferSize < totalVertexSize) {
            if (fd.vertexBuffer != nullptr) {
                ctx.DestroyBuffer(fd.vertexBuffer);
            }
            fd.vertexBufferSize = totalVertexSize;
            ResourceCreationContext::BufferCreateInfo ci;
            ci.memoryProperties = MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
            ci.size = totalVertexSize;
            ci.usage = BufferUsageFlags::VERTEX_BUFFER_BIT;
            fd.vertexBuffer = ctx.CreateBuffer(ci);
            fd.vertexBufferMapped = ctx.MapBuffer(fd.vertexBuffer, 0, totalVertexSize);
        }
        if (fd.indexBuffer == nullptr || fd.indexBufferSize < totalIndexSize) {
            if (fd.indexBuffer != nullptr) {
                ctx.DestroyBuffer(fd.indexBuffer);
            }
            fd.indexBufferSize = totalIndexSize;
            ResourceCreationContext::BufferCreateInfo ci;
            ci.memoryProperties = MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT;
            ci.size = totalIndexSize;
            ci.usage = BufferUsageFlags::INDEX_BUFFER_BIT;
            fd.indexBuffer = ctx.CreateBuffer(ci);
            fd.indexBufferMapped = ctx.MapBuffer(fd.indexBuffer, 0, totalIndexSize);
        }
        resourceCreationFinished.Signal();
    });
    resourceCreationFinished.Wait();
}

void UiRenderSystem::RenderUi(FrameContext & context, CommandBuffer * commandBuffer)
{
    OPTICK_EVENT();
    auto data = context.imguiDrawData;
    auto & fd = frameData[context.currentGpuFrameIndex];

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
                commandBuffer->CmdDrawIndexed(imCmdBuf->ElemCount, 1, currIndexPos, currVertexPos, 0);
            }
            currIndexPos += imCmdBuf->ElemCount;
        }
        currVertexPos += cmdList->VtxBuffer.Size;
    }
}
