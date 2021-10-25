#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamShaderLoader
    
    Resource loader to setup a Shader object from a stream.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/VkShaderCache.h"
namespace CoreGraphics
{
class ShaderCache : public Vulkan::VkShaderCache
{
    __DeclareClass(ShaderCache);
};
}
#else
#error "StreamShaderLoader class not implemented on this platform!"
#endif

