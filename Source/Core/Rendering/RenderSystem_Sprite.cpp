#include "RenderSystem.h"

#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/Semaphore.h"

SpriteInstanceId RenderSystem::CreateSpriteInstance(Image * image)
{
    sprites.emplace_back();
    auto id = sprites.size() - 1;
    sprites[id].id = id;

    Semaphore sem;
    renderer->CreateResources([this, &sem, id, image](ResourceCreationContext & ctx) {
        auto layout =
            ResourceManager::GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/spritePt.layout");
        this->sprites[id].uniformBuffer =
            ctx.CreateBuffer({sizeof(glm::mat4),
                              BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
                              DEVICE_LOCAL_BIT});

        ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uvDescriptor = {
            this->sprites[id].uniformBuffer, 0, sizeof(glm::mat4)};

        auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");

        ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {sampler,
                                                                                           image->GetDefaultView()};

        ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
            {DescriptorType::UNIFORM_BUFFER, 0, uvDescriptor},
            {DescriptorType::COMBINED_IMAGE_SAMPLER, 1, imgDescriptor}};

        this->sprites[id].descriptorSet = ctx.CreateDescriptorSet({2, descriptors, layout});
        sem.Signal();
    });
    sem.Wait();
    return id;
}

void RenderSystem::DestroySpriteInstance(SpriteInstanceId spriteInstance)
{
    auto sprite = GetSpriteInstance(spriteInstance);
    auto descriptorSet = sprite->descriptorSet;
    auto uniforms = sprite->uniformBuffer;
    this->DestroyResources([descriptorSet, uniforms](ResourceCreationContext & ctx) {
        ctx.DestroyDescriptorSet(descriptorSet);
        ctx.DestroyBuffer(uniforms);
    });
}

SpriteInstance * RenderSystem::GetSpriteInstance(SpriteInstanceId id)
{
    return &sprites[id];
}
