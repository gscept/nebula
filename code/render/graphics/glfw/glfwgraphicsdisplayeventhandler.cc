//------------------------------------------------------------------------------
//  glfwgraphicsdisplayeventhandler.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphics/glfw/glfwgraphicsdisplayeventhandler.h"
#include "graphics/graphicsserver.h"

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
    switch (displayEvent.GetEventCode())
    {
        case DisplayEvent::WindowResized:
            graphicsServer->OnWindowResized(displayEvent.GetWindowId());
            return true;
        default:
            return false;
    }
}
    
} // namespace GLFW