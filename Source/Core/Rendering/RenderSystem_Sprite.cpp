#include "RenderSystem.h"

#include <glm/gtc/type_ptr.hpp>
#include <optick/optick.h>

#include "Core/FrameContext.h"
#include "Core/Resources/Image.h"
#include "Core/Resources/ResourceManager.h"
#include "RenderingBackend/Abstract/RenderResources.h"
#include "RenderingBackend/Abstract/ResourceCreationContext.h"
#include "Util/Semaphore.h"

SpriteInstanceId RenderSystem::CreateSpriteInstance(Image * image, bool isActive)
{
    sprites.emplace_back();
    auto id = sprites.size() - 1;
    sprites[id].id = id;
    sprites[id].isActive = isActive;

    Semaphore sem;
    this->CreateResources([this, &sem, id, image](ResourceCreationContext & ctx) {
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
    sprite->isActive = false;
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

void RenderSystem::PreRenderSprites(FrameContext const & context, std::vector<UpdateSpriteInstance> const & sprites)
{
    OPTICK_EVENT();
    auto & currFrame = frameInfo[context.currentGpuFrameIndex];

    for (auto const & sprite : sprites) {
        auto spriteInstance = GetSpriteInstance(sprite.spriteInstance);
        spriteInstance->isActive = sprite.isActive;
        currFrame.preRenderPassCommandBuffer->CmdUpdateBuffer(
            spriteInstance->uniformBuffer, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(sprite.localToWorld));
        if (sprite.newImage) {
            auto oldDescriptorSet = spriteInstance->descriptorSet;
            Semaphore sem;
            this->CreateResources([spriteInstance, &sprite, &sem](ResourceCreationContext & ctx) {
                ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uvDescriptor = {
                    spriteInstance->uniformBuffer, 0, sizeof(glm::mat4)};

                auto layout = ResourceManager::GetResource<DescriptorSetLayoutHandle>(
                    "_Primitives/DescriptorSetLayouts/spritePt.layout");
                auto sampler = ResourceManager::GetResource<SamplerHandle>("_Primitives/Samplers/Default.sampler");

                ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {
                    sampler, sprite.newImage->GetDefaultView()};

                ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
                    {DescriptorType::UNIFORM_BUFFER, 0, uvDescriptor},
                    {DescriptorType::COMBINED_IMAGE_SAMPLER, 1, imgDescriptor}};

                spriteInstance->descriptorSet = ctx.CreateDescriptorSet({2, descriptors, layout});
                sem.Signal();
            });
            sem.Wait();
            if (oldDescriptorSet) {
                this->DestroyResources(
                    [oldDescriptorSet](ResourceCreationContext & ctx) { ctx.DestroyDescriptorSet(oldDescriptorSet); });
            }
        }
    }
}