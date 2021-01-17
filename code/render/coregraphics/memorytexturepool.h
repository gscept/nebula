#pragma once
//------------------------------------------------------------------------------
/**
    Platform-wrapper for memory texture loader
    
    @copyright
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vkmemorytexturepool.h"
namespace CoreGraphics
{
class MemoryTexturePool : public Vulkan::VkMemoryTexturePool
{
    __DeclareClass(MemoryTexturePool);
};
}
#else
#error "memorytextureloader is not implemented on this configuration"
#endif