#include "FrameContext.h"

#include <cstdlib>

#include <imgui.h>

void FrameContext::Destroy(FrameContext & context)
{
    if (context.imguiDrawData) {
        for (int i = 0; i < context.imguiDrawData->CmdListsCount; ++i) {
            context.imguiDrawData->CmdLists[i]->ClearFreeMemory();
        }
        free(context.imguiDrawData->CmdLists);
        delete context.imguiDrawData;
    }
}
