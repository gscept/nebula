#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::RenderDevice
  
    The central rendering object of the Nebula3 core graphics system. This
    is basically an encapsulation of the Direct3D device. The render device
    will presents its backbuffer to the display managed by the
    CoreGraphics::DisplayDevice singleton.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#if __DX11__
#include "coregraphics/d3d11/d3d11renderdevice.h"
namespace CoreGraphics
{
class RenderDevice : public Direct3D11::D3D11RenderDevice
{
	__DeclareClass(RenderDevice);
	__DeclareSingleton(RenderDevice);
public:
	/// constructor
	RenderDevice();
	/// destructor
	virtual ~RenderDevice();
};
} // namespace CoreGraphics
#elif __OGL4__
#include "coregraphics/ogl4/ogl4renderdevice.h"
namespace CoreGraphics
{
class RenderDevice : public OpenGL4::OGL4RenderDevice
{
	__DeclareClass(RenderDevice);
	__DeclareSingleton(RenderDevice);
public:
	/// constructor
	RenderDevice();
	/// destructor
	virtual ~RenderDevice();
};
} // namespace CoreGraphics
#elif __VULKAN__
#include "coregraphics/vk/vkrenderdevice.h"
namespace CoreGraphics
{
class RenderDevice : public Vulkan::VkRenderDevice
{
	__DeclareClass(RenderDevice);
	__DeclareSingleton(RenderDevice);
public:
	/// constructor
	RenderDevice();
	/// destructor
	virtual ~RenderDevice();
};
} // namespace CoreGraphics
#elif __DX9__
#include "coregraphics/d3d9/d3d9renderdevice.h"
namespace CoreGraphics
{
class RenderDevice : public Direct3D9::D3D9RenderDevice
{
    __DeclareClass(RenderDevice);
    __DeclareSingleton(RenderDevice);
public:
    /// constructor
    RenderDevice();
    /// destructor
    virtual ~RenderDevice();
};
} // namespace CoreGraphics
#else
#error "RenderDevice class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
