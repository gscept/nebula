//------------------------------------------------------------------------------
//  displaydevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/displaydevice.h"

namespace CoreGraphics
{
#if __DX11__
__ImplementClass(CoreGraphics::DisplayDevice, 'DDVC', Direct3D11::D3D11DisplayDevice);
__ImplementSingleton(CoreGraphics::DisplayDevice);
#elif __OGL4__ ||  __VULKAN__
__ImplementClass(CoreGraphics::DisplayDevice, 'DDVC', GLFW::GLFWDisplayDevice);
__ImplementSingleton(CoreGraphics::DisplayDevice);
#elif __DX9__
__ImplementClass(CoreGraphics::DisplayDevice, 'DDVC', Direct3D9::D3D9DisplayDevice);
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

