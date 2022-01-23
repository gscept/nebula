#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::MemoryTextureCache
    
    Platform-wrapper for memory texture loader
    
    @copyright
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vkmemorytexturecache.h"
namespace CoreGraphics
{
class MemoryTextureCache : public Vulkan::VkMemoryTextureCache
{
    __DeclareClass(MemoryTextureCache);
};
}
#else
#error "memorytextureloader is not implemented on this configuration"
#endif
