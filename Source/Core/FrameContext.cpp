#include "FrameContext.h"

#include <cstdlib>

#include <ThirdParty/imgui/imgui.h>

void FrameContext::Destroy(FrameContext & context)
{
    if (context.imguiDrawData) {
        for (int i = 0; i < context.imguiDrawData->CmdListsCount; ++i) {
            IM_DELETE(context.imguiDrawData->CmdLists[i]);
        }
        free(context.imguiDrawData->CmdLists);
        delete context.imguiDrawData;
    }
}
