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

    VkImageViewCreateInfo viewCreate =
    {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        img,
        type,
        format,
        VkTypes::AsVkMapping(loadInfo.format),
        viewRange
    };
    VkResult stat = vkCreateImageView(loadInfo.dev, &viewCreate, nullptr, &runtimeInfo.view);
    n_assert(stat == VK_SUCCESS);

    TextureViewId ret;
    ret.id24 = id;
    ret.id8 = TextureViewIdType;
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
    vkDestroyImageView(loadInfo.dev, runtimeInfo.view, nullptr);
    textureViewAllocator.Dealloc(id.id24);
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
