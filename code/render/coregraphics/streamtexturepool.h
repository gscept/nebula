#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamTextureLoader
  
    Resource loader for loading texture data from a Nebula stream. Supports
    synchronous and asynchronous loading.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#if __VULKAN__
#include "coregraphics/vk/vkstreamtexturepool.h"
namespace CoreGraphics
{
class StreamTexturePool : public Vulkan::VkStreamTexturePool
{
	__DeclareClass(StreamTexturePool);
};
}
#else
#error "StreamTextureLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------


