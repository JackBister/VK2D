#pragma once

#include <cstdint>
#include <functional>

#include "Core/Rendering/Backend/Abstract/RenderResources.h"
#include "Core/Rendering/Backend/Abstract/RendererConfig.h"
#include "Core/Rendering/Backend/Abstract/RendererProperties.h"
#include "Core/Rendering/Backend/Abstract/ResourceCreationContext.h"

/**
 * Interface that a rendering backend (like OpenGL, DirectX, Vulkan) should implement.
 */
class IRenderer
{
public:
    /**
     * Returns the format of the backbuffers. When rendering into the framebuffers returned by CreateBackbuffers, this
     * is the format that should be used.
     */
    virtual Format GetBackbufferFormat() const = 0;
    /**
     * Returns the rendering resolution (currently tied to window resolution) of the renderer's backbuffers
     */
    virtual glm::ivec2 GetResolution() const = 0;
    /**
     * Returns the number of framebuffers in the swapchain.
     */
    virtual uint32_t GetSwapCount() const = 0;

    /**
     * Executes resource creation commands contained in the given function.
     * There is no guarantee that these commands should run synchronously. The renderer is free to run them on a
     * separate thread and the consumer is responsible for making sure any created resources are not used before the
     * function has ran.
     */
    virtual void CreateResources(std::function<void(ResourceCreationContext &)> fun) = 0;
    /**
     * Gets the backbuffers in the swapchain. The returned ImageViews should be suitable for using as color output
     * attachments in a renderpass/framebuffer. This should be called after calling RecreateSwapchain. The returned
     * vector must have the same size as the return value of GetSwapCount().
     */
    virtual std::vector<ImageViewHandle *> GetBackbuffers() = 0;

    /**
     * Takes ownership of the next frame in the swapchain. Once acquired the consumer can render freely into the
     * backbuffer at the given index. If signalSem and/or signalFence are given, they will be signalled when the frame
     * is acquired. signalSem or signalFence MUST be given, it is not valid to call AcquireNextFrameIndex(nullptr,
     * nullptr).
     * If the swapchain has become outdated (for example when the window has changed size), UINT32_MAX will be returned.
     * The consumer should call RecreateSwapchain in response to this followed by recreating any consumer-side resources
     * that depend on the renderer's backbuffers and resolution (such as renderpasses and pipelines).
     */
    virtual uint32_t AcquireNextFrameIndex(SemaphoreHandle * signalSem, FenceHandle * signalFence) = 0;
    /**
     * Executes a command buffer.
     * Before executing, waits for all semaphores in waitSem to be signalled.
     * After executing, signals all semaphores in signalSem and signalFence if given.
     */
    virtual void ExecuteCommandBuffer(CommandBuffer * ctx, std::vector<SemaphoreHandle *> waitSem,
                                      std::vector<SemaphoreHandle *> signalSem,
                                      FenceHandle * signalFence = nullptr) = 0;
    /**
     * Presents the image at imageIndex to the screen. imageIndex must be an image currently owned by the consumer
     * (acquired via AcquireNextFrameIndex)
     * Waits for waitSem to be signalled before presenting.
     */
    virtual void SwapWindow(uint32_t imageIndex, SemaphoreHandle * waitSem) = 0;

    /**
     * Recreate the swap chain to match the current window. Should be called in response to AcquireNextFrameIndex
     * returning UINT32_MAX. The consumer should not have ownership of any images in the swapchain when this is called.
     */
    virtual void RecreateSwapchain() = 0;

    /**
     * Gets the current rendering configuration.
     */
    virtual RendererConfig GetConfig() = 0;

    /**
     * Update the rendering configuration.
     */
    virtual void UpdateConfig(RendererConfig) = 0;

    /**
     * Gets the renderer properties for this renderer. Renderer properties are immutable facts about the rendering
     * backend. Once the renderer has been initialized it is therefore safe to save a reference to this return value.
     */
    virtual RendererProperties const & GetProperties() = 0;
};
