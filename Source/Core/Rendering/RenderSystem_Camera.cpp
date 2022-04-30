#include "RenderSystem.h"

#include "Core/Resources/ResourceManager.h"
#include "RenderingBackend/Renderer.h"
#include "Util/Semaphore.h"

CameraInstanceId RenderSystem::CreateCamera(bool isActive)
{
    // TODO: multithread danger?
    cameras.emplace_back();
    auto id = cameras.size() - 1;
    cameras[id].id = id;
    cameras[id].isActive = isActive;
    Semaphore sem;
    renderer->CreateResources([this, &sem, id](ResourceCreationContext & ctx) {
        auto layout =
            ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/cameraPt.layout");
        this->cameras[id].uniformBuffer =
            ctx.CreateBuffer({2 * sizeof(glm::mat4),
                              BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                              DEVICE_LOCAL_BIT});

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptor = {
            this->cameras[id].uniformBuffer, 0, 2 * sizeof(glm::mat4)};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, uniformDescriptor}};

        this->cameras[id].descriptorSet = ctx.CreateDescriptorSet({1, descriptors, layout});
        sem.Signal();
    });
    sem.Wait();
    return id;
}

void RenderSystem::DestroyCamera(CameraInstanceId cameraHandle)
{
    auto cam = GetCamera(cameraHandle);
    cam->isActive = false;
    auto descriptorSet = cam->descriptorSet;
    auto uniforms = cam->uniformBuffer;
    this->DestroyResources([descriptorSet, uniforms](ResourceCreationContext & ctx) {
        ctx.DestroyDescriptorSet(descriptorSet);
        ctx.DestroyBuffer(uniforms);
    });
}

CameraInstance * RenderSystem::GetCamera(CameraInstanceId id)
{
    return &this->cameras[id];
}
