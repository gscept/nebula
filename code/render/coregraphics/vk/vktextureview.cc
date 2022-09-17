//------------------------------------------------------------------------------
//  vktextureview.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "vktextureview.h"
#include "coregraphics/textureview.h"
#include "vktexture.h"
#include "vktypes.h"
#include "vkgraphicsdevice.h"


namespace Vulkan
{
using namespace CoreGraphics;

VkTextureViewAllocator textureViewAllocator(0x00FFFFFF);


//------------------------------------------------------------------------------
/**
*/
const VkImageView 
TextureViewGetVk(const TextureViewId id)
{
    return textureViewAllocator.Get<TextureView_RuntimeInfo>(id.id24).view;
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice
TextureViewGetVkDevice(const CoreGraphics::TextureViewId id)
{
    return textureViewAllocator.Get<TextureView_LoadInfo>(id.id24).dev;
}

} // namespace Vulkan


namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
TextureViewId 
CreateTextureView(const TextureViewCreateInfo& info)
{
    Ids::Id32 id = textureViewAllocator.Alloc();
    VkTextureViewRuntimeInfo& runtimeInfo = textureViewAllocator.Get<TextureView_RuntimeInfo>(id);
    VkTextureViewLoadInfo& loadInfo = textureViewAllocator.Get<TextureView_LoadInfo>(id);

    loadInfo.dev = CoreGraphics::GetCurrentDevice();
    loadInfo.tex = info.tex;
    loadInfo.format = info.format;
    loadInfo.mip = info.startMip;
    loadInfo.numMips = info.numMips;
    loadInfo.layer = info.startLayer;
    loadInfo.numLayers = info.numLayers;
    loadInfo.swizzle = info.swizzle;

    bool isDepthFormat = VkTypes::IsDepthFormat(info.format);
    VkImageSubresourceRange viewRange;
    viewRange.aspectMask = isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT; // view only supports reading depth in shader
    viewRange.baseMipLevel = info.startMip;
    viewRange.levelCount = info.numMips;
    viewRange.baseArrayLayer = info.startLayer;
    viewRange.layerCount = info.numLayers;

    VkImage img = TextureGetVkImage(info.tex);
    VkImageViewType type = VkTypes::AsVkImageViewType(TextureGetType(info.tex));
    VkFormat format = VkTypes::AsVkFormat(info.format);

    VkComponentMapping mapping;
    mapping.r = VkSwizzle[(uint)info.swizzle.red];
    mapping.g = VkSwizzle[(uint)info.swizzle.green];
    mapping.b = VkSwizzle[(uint)info.swizzle.blue];
    mapping.a = VkSwizzle[(uint)info.swizzle.alpha];

    VkImageViewCreateInfo viewCreate =
    {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        img,
        type,
        format,
        mapping,
        viewRange
    };
    VkResult stat = vkCreateImageView(loadInfo.dev, &viewCreate, nullptr, &runtimeInfo.view);
    n_assert(stat == VK_SUCCESS);

    TextureViewId ret;
    ret.id24 = id;
    ret.id8 = TextureViewIdType;

#if NEBULA_GRAPHICS_DEBUG
    ObjectSetName(ret, info.name.Value());
#endif

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyTextureView(const TextureViewId id)
{
    VkTextureViewLoadInfo& loadInfo = textureViewAllocator.Get<TextureView_LoadInfo>(id.id24);
    VkTextureViewRuntimeInfo& runtimeInfo = textureViewAllocator.Get<TextureView_RuntimeInfo>(id.id24);
    CoreGraphics::DelayedDeleteTextureView(id);
    textureViewAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
TextureViewReload(const TextureViewId id)
{
    VkTextureViewRuntimeInfo& runtimeInfo = textureViewAllocator.Get<TextureView_RuntimeInfo>(id.id24);
    VkTextureViewLoadInfo& loadInfo = textureViewAllocator.Get<TextureView_LoadInfo>(id.id24);

    // First destroy the old view
    CoreGraphics::DelayedDeleteTextureView(id);

    bool isDepthFormat = VkTypes::IsDepthFormat(loadInfo.format);
    VkImageSubresourceRange viewRange;
    viewRange.aspectMask = isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT; // view only supports reading depth in shader
    viewRange.baseMipLevel = loadInfo.mip;
    viewRange.levelCount = loadInfo.numMips;
    viewRange.baseArrayLayer = loadInfo.layer;
    viewRange.layerCount = loadInfo.numLayers;

    VkImage img = TextureGetVkImage(loadInfo.tex);
    VkImageViewType type = VkTypes::AsVkImageViewType(TextureGetType(loadInfo.tex));
    VkFormat format = VkTypes::AsVkFormat(loadInfo.format);

    VkComponentMapping mapping;
    mapping.r = VkSwizzle[(uint)loadInfo.swizzle.red];
    mapping.g = VkSwizzle[(uint)loadInfo.swizzle.green];
    mapping.b = VkSwizzle[(uint)loadInfo.swizzle.blue];
    mapping.a = VkSwizzle[(uint)loadInfo.swizzle.alpha];

    VkImageViewCreateInfo viewCreate =
    {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        img,
        type,
        format,
        mapping,
        viewRange
    };
    VkResult stat = vkCreateImageView(loadInfo.dev, &viewCreate, nullptr, &runtimeInfo.view);
    n_assert(stat == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
TextureId
TextureViewGetTexture(const TextureViewId id)
{
    VkTextureViewLoadInfo& loadInfo = textureViewAllocator.Get<TextureView_LoadInfo>(id.id24);
    return loadInfo.tex;
}

} // namespace CoreGraphics
