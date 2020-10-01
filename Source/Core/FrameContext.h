#pragma once

#include <cstdint>

class ImDrawData;

struct RenderStats {
    uint32_t numMeshBatches = 0;
    uint32_t numTransparentMeshBatches = 0;
};

/**
 * FrameContext is an object which lives for the duration of a frame, from the point where GameModule::Tick is called
 * to just after the frame is submitted to the swap chain. It contains values which may be useful for any part of the
 * frame which would be too annoying to pass around separately.
 * Some values in the FrameContext may not be valid until a certain stage of the frame.
 */
struct FrameContext {
    static void Destroy(FrameContext & context);

    // frameNumber is a number which is incremented by 1 for each completed frame. Note that if you have a FrameContext
    // with frameNumber=N, there may still be another thread which has a FrameContext with frameNumber=N-1
    // This value is valid at all points of the frame.
    uint64_t frameNumber = 0;

    // previousGpuFrameIndex is used by RenderSystem. It is valid after renderSystem->StartFrame has been called.
    uint32_t previousGpuFrameIndex = 0;
    // currentGpuFrameIndex is used by RenderSystem. It is valid at all points of the frame, but only RenderSystem
    // should use it before renderSystem->StartFrame is called
    uint32_t currentGpuFrameIndex = 0;

    // renderStats contains statistics from rendering this frame. It will be fully filled after renderSystem->EndFrame
    // is finished. In the future the stats should be shown graphically somewhere in the editor, but for now they are
    // only available in the debugger.
    RenderStats renderStats;

    // imguiDrawData contains a copy of the draw lists from Imgui for this frame. It is valid after
    // renderSystem->PreRenderFrame has been called.
    // This is used because another thread might call ImGui::NewFrame, which makes ImGui::GetDrawData return invalid
    // data, so each frame needs to store its own copy of the draw data.
    ImDrawData * imguiDrawData = nullptr;
};
