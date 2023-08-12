//------------------------------------------------------------------------------
//  displaydevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/displaydevice.h"

namespace CoreGraphics
{
#if __VULKAN__
__ImplementClass(CoreGraphics::DisplayDevice, 'DDVC', GLFW::GLFWDisplayDevice);
__ImplementSingleton(CoreGraphics::DisplayDevice);
#else
#error "DisplayDevice class not implemented on this platform!"
#endif

//------------------------------------------------------------------------------
/**
*/
DisplayDevice::DisplayDevice()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
DisplayDevice::~DisplayDevice()
{
    __DestructSingleton;
}

} // namespace CoreGraphics

