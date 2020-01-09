#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ShaderServer
  
    The ShaderServer object loads the available shaders and can instantiate
    shaders for usage.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __DX11__
#include "coregraphics/d3d11/d3d11shaderserver.h"
namespace CoreGraphics
{
class ShaderServer : public Direct3D11::D3D11ShaderServer
{
	__DeclareClass(ShaderServer);
	__DeclareSingleton(ShaderServer);
public:
	/// constructor
	ShaderServer();
	/// destructor
	virtual ~ShaderServer();
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4shaderserver.h"
namespace CoreGraphics
{
class ShaderServer : public OpenGL4::OGL4ShaderServer
{
	__DeclareClass(ShaderServer);
	__DeclareSingleton(ShaderServer);
public:
	/// constructor
	ShaderServer();
	/// destructor
	virtual ~ShaderServer();
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkshaderserver.h"
namespace CoreGraphics
{
class ShaderServer : public Vulkan::VkShaderServer
{
	__DeclareClass(ShaderServer);
	__DeclareSingleton(ShaderServer);
public:
	/// constructor
	ShaderServer();
	/// destructor
	virtual ~ShaderServer();
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9shaderserver.h"
namespace CoreGraphics
{
class ShaderServer : public Direct3D9::D3D9ShaderServer
{
    __DeclareClass(ShaderServer);
    __DeclareSingleton(ShaderServer);
public:
    /// constructor
    ShaderServer();
    /// destructor
    virtual ~ShaderServer();
};
}
#else
#error "ShaderServer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
