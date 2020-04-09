#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ShaderServer
  
    The ShaderServer object loads the available shaders and can instantiate
    shaders for usage.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __VULKAN__
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
#else
#error "ShaderServer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
