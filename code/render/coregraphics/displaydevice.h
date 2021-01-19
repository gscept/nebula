#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::DisplayDevice

    A DisplayDevice object represents the display where the RenderDevice
    presents the rendered frame. 

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/glfw/glfwdisplaydevice.h"
namespace CoreGraphics
{
class DisplayDevice : public GLFW::GLFWDisplayDevice
{
    __DeclareClass(DisplayDevice);
    __DeclareSingleton(DisplayDevice);
public:
    /// constructor
    DisplayDevice();
    /// destructor
    virtual ~DisplayDevice();
};
} // namespace CoreGraphics
#else
#error "CoreGraphics::DisplayDevice not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

