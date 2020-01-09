#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::DisplayDevice

	A DisplayDevice object represents the display where the RenderDevice
	presents the rendered frame. 

	(C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11displaydevice.h"
namespace CoreGraphics
{
class DisplayDevice : public Direct3D11::D3D11DisplayDevice
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
#elif __OGL4__ ||  __VULKAN__
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
#elif __DX9__
#include "coregraphics/d3d9/d3d9displaydevice.h"
namespace CoreGraphics
{
class DisplayDevice : public Direct3D9::D3D9DisplayDevice
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

