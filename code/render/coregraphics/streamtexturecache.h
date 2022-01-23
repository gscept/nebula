#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamTextureLoader
  
    Resource loader for loading texture data from a Nebula stream. Supports
    synchronous and asynchronous loading.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __VULKAN__
#include "coregraphics/vk/vkstreamtexturecache.h"
namespace CoreGraphics
{
class StreamTextureCache : public Vulkan::VkStreamTextureCache
{
    __DeclareClass(StreamTextureCache);
};
}
#else
#error "StreamTextureLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


