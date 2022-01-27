//------------------------------------------------------------------------------
// vktexture.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vktexture.h"
#include "vktextureview.h"
#include "vkloader.h"
#include "io/stream.h"

namespace Vulkan
{

VkTextureAllocator textureAllocator(0x00FFFFFF);
VkTextureStencilExtensionAllocator textureStencilExtensionAllocator(0x00FFFFFF);
VkTextureSwapExtensionAllocator textureSwapExtensionAllocator(0x00FFFFFF);
VkTextureSparseExtensionAllocator textureSparseExtensionAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkImage
TextureGetVkImage(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).img;
}

//------------------------------------------------------------------------------
/**
*/
const VkImageView
TextureGetVkImageView(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_RuntimeInfo>(id.resourceId).view;
}

//------------------------------------------------------------------------------
/**
*/
const VkImageView 
TextureGetVkStencilImageView(const CoreGraphics::TextureId id)
{
    Ids::Id32 stencil = textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).stencilExtension;
    n_assert(stencil != Ids::InvalidId32);
    return TextureViewGetVk(textureStencilExtensionAllocator.GetUnsafe<TextureExtension_StencilInfo>(stencil));
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice 
TextureGetVkDevice(const CoreGraphics::TextureId id)
{
    return textureAllocator.GetUnsafe<Texture_LoadInfo>(id.resourceId).dev;
}

} // namespace Vulkan
