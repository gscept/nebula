//------------------------------------------------------------------------------
//  graphicsdisplayeventhandler.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "graphics/graphicsdisplayeventhandler.h"
#include "graphics/graphicsserver.h"

#if __VULKAN__
#include "coregraphics/vk/vkpipelinedatabase.h"
#endif

namespace Graphics
{
__ImplementClass(Graphics::GraphicsDisplayEventHandler, 'WGEH', CoreGraphics::DisplayEventHandler);

using namespace Input;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
bool
GraphicsDisplayEventHandler::HandleEvent(const DisplayEvent& displayEvent)
{
    Ptr<Graphics::GraphicsServer> graphicsServer = Graphics::GraphicsServer::Instance();
#if __VULKAN__
    Vulkan::VkPipelineDatabase* pipelineDatabase = Vulkan::VkPipelineDatabase::Instance();
#endif
    switch (displayEvent.GetEventCode())
    {
        case DisplayEvent::CloseRequested:
        {
            CoreGraphics::WindowId wnd = displayEvent.GetWindowId();
            if (wnd.id != 0)
            {
                // If the window has user data, assume it's being managed elsewhere
                void* userData = CoreGraphics::WindowGetUserData(wnd);
                if (userData == nullptr)
                {
                    Graphics::GraphicsServer::Instance()->RemoveWindow(displayEvent.GetWindowId());
                    CoreGraphics::DestroyWindow(displayEvent.GetWindowId());
                    return true;
                }
            }
            break;
        }
        case DisplayEvent::WindowResized:
        {
            // Run all contexts resize calls
            if (graphicsServer->OnWindowResized(displayEvent.GetWindowId()))
            {
                // Invalidate pipelines because passes have changed
                pipelineDatabase->RecreatePipelines();
            }

            return true;
        }
        default:
            return false;
    }
    return false;
}
    
} // namespace GLFW
