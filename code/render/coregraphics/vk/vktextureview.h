#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of a texture view

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/textureview.h"
namespace Vulkan
{

struct VkTextureViewLoadInfo
{
    VkDevice dev;
    CoreGraphics::TextureId tex;
    IndexT mip;
    SizeT numMips;
    IndexT layer;
    SizeT numLayers;
    CoreGraphics::PixelFormat::Code format;
};

struct VkTextureViewRuntimeInfo
{
    VkImageView view;
};

enum
{
    TextureView_LoadInfo,
    TextureView_RuntimeInfo
};

typedef Ids::IdAllocator<
    VkTextureViewLoadInfo,
    VkTextureViewRuntimeInfo
> VkTextureViewAllocator;
extern VkTextureViewAllocator textureViewAllocator;

/// Get vk image view
const VkImageView TextureViewGetVk(const CoreGraphics::TextureViewId id);
/// Get device used to create view
const VkDevice TextureViewGetVkDevice(const CoreGraphics::TextureViewId id);

} // namespace Vulkan
