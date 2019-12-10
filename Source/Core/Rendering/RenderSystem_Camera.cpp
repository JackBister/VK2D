#include "RenderSystem.h"

#include "Core/Resources/ResourceManager.h"
#include "Core/Semaphore.h"

CameraHandle RenderSystem::CreateCamera()
{
    // TODO: multithread danger?
    cameras.emplace_back();
    auto id = cameras.size() - 1;
    Semaphore sem;
    renderer->CreateResources([this, &sem, id](ResourceCreationContext & ctx) {
        auto layout =
            ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraPt.layout");
        this->cameras[id].uniformBuffer =
            ctx.CreateBuffer({sizeof(glm::mat4),
                              BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                              DEVICE_LOCAL_BIT});

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptor = {
            this->cameras[id].uniformBuffer, 0, sizeof(glm::mat4)};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, uniformDescriptor}};

        this->cameras[id].descriptorSet = ctx.CreateDescriptorSet({1, descriptors, layout});
        sem.Signal();
    });
    sem.Wait();
    CameraHandle ret;
    ret.id = id;
    return ret;
}

void RenderSystem::DestroyCamera(CameraHandle cameraHandle)
{
    auto cam = GetCamera(cameraHandle);
    auto descriptorSet = cam->descriptorSet;
    auto uniforms = cam->uniformBuffer;
    this->DestroyResources([descriptorSet, uniforms](ResourceCreationContext & ctx) {
        ctx.DestroyDescriptorSet(descriptorSet);
        ctx.DestroyBuffer(uniforms);
    });
}

CameraResources * RenderSystem::GetCamera(CameraHandle cameraHandle)
{
    return &this->cameras[cameraHandle.id];
}
