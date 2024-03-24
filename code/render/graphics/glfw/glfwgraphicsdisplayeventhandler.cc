//------------------------------------------------------------------------------
//  glfwgraphicsdisplayeventhandler.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "graphics/glfw/glfwgraphicsdisplayeventhandler.h"
#include "graphics/graphicsserver.h"

#if __VULKAN__
#include "coregraphics/vk/vkpipelinedatabase.h"
#endif

namespace GLFW
{
__ImplementClass(GLFW::GLFWGraphicsDisplayEventHandler, 'WGEH', CoreGraphics::DisplayEventHandler);

using namespace Input;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
bool
GLFWGraphicsDisplayEventHandler::HandleEvent(const DisplayEvent& displayEvent)
{
    Ptr<Graphics::GraphicsServer> graphicsServer = Graphics::GraphicsServer::Instance();
    Ptr<Frame::FrameServer> frameServer = Frame::FrameServer::Instance();
#if __VULKAN__
    Vulkan::VkPipelineDatabase* pipelineDatabase = Vulkan::VkPipelineDatabase::Instance();
#endif
    switch (displayEvent.GetEventCode())
    {
        case DisplayEvent::WindowResized:
        {
            // Invalidate pipelines
            pipelineDatabase->RecreatePipelines();

            // Run all contexts resize calls
            graphicsServer->OnWindowResized(displayEvent.GetWindowId());

            // Finally, 
            frameServer->OnWindowResize();
            return true;
        }
        default:
            return false;
    }
}
    
} // namespace GLFW
