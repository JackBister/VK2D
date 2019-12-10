#include "RenderSystem.h"

#include "Core/Resources/ResourceManager.h"

StaticMeshInstance RenderSystem::CreateStaticMeshInstance()
{
    staticMeshes.emplace_back();
    auto id = staticMeshes.size() - 1;
    Semaphore sem;
    renderer->CreateResources([this, &sem, id](ResourceCreationContext & ctx) {
        auto layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
            "_Primitives/DescriptorSetLayouts/modelMesh.layout");
        this->staticMeshes[id].uniformBuffer =
            ctx.CreateBuffer({sizeof(glm::mat4),
                              BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                              DEVICE_LOCAL_BIT});

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uniformDescriptor = {
            this->staticMeshes[id].uniformBuffer, 0, sizeof(glm::mat4)};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, uniformDescriptor}};

        this->staticMeshes[id].descriptorSet = ctx.CreateDescriptorSet({1, descriptors, layout});

        sem.Signal();
    });
    sem.Wait();
    StaticMeshInstance ret;
    ret.id = id;
    return ret;
}

void RenderSystem::DestroyStaticMeshInstance(StaticMeshInstance staticMesh)
{
    auto mesh = GetStaticMeshInstance(staticMesh);
    auto descriptorSet = mesh->descriptorSet;
    auto uniforms = mesh->uniformBuffer;
    this->DestroyResources([descriptorSet, uniforms](ResourceCreationContext & ctx) {
        ctx.DestroyDescriptorSet(descriptorSet);
        ctx.DestroyBuffer(uniforms);
    });
}

StaticMeshInstanceResources * RenderSystem::GetStaticMeshInstance(StaticMeshInstance staticMesh)
{
    return &staticMeshes[staticMesh.id];
}
