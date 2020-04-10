#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamShaderLoader
    
    Resource loader to setup a Shader object from a stream.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vkshaderpool.h"
namespace CoreGraphics
{
class ShaderPool : public Vulkan::VkShaderPool
{
	__DeclareClass(ShaderPool);
};
}
#else
#error "StreamShaderLoader class not implemented on this platform!"
#endif

    