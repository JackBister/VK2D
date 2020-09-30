#include "FrameContext.h"

#include <imgui.h>

void FrameContext::Destroy(FrameContext & context)
{
    if (context.imguiDrawData) {
        for (int i = 0; i < context.imguiDrawData->CmdListsCount; ++i) {
            context.imguiDrawData->CmdLists[i]->ClearFreeMemory();
        }
        delete context.imguiDrawData;
    }
}
