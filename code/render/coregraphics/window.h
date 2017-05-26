#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::Window

	A window represents a renderable surface.

	(C) 2007 Radon Labs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11window.h"
namespace CoreGraphics
{
	class Window : public Direct3D11::D3D11Window
	{
		__DeclareClass(Window);
	};
} // namespace CoreGraphics
#elif __OGL4__ || __VULKAN__
#include "coregraphics/glfw/glfwwindow.h"
namespace CoreGraphics
{
	class Window : public GLFW::GLFWWindow
	{
		__DeclareClass(Window);
	};
} // namespace CoreGraphics
#elif __DX9__
#include "coregraphics/d3d9/d3d9window.h"
namespace CoreGraphics
{
	class Window : public Direct3D9::D3D9Window
	{
		__DeclareClass(Window);
	};
} // namespace CoreGraphics
#else
#error "CoreGraphics::Window not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

