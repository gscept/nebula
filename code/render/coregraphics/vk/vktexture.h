#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan texture abstraction types.

    For the actual loader code, see VkStreamTextureLoader and VkMemoryTextureLoader.
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/texture.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/textureview.h"
#include "ids/idallocator.h"
#include "vkmemory.h"
#include "coregraphics/load/glimltypes.h"

namespace IO
{
class Stream;
};

namespace Vulkan
{
struct VkTextureLoadInfo : CoreGraphics::TextureCreateInfoAdjusted
{
    VkDevice dev;
    VkImage img;
    CoreGraphics::Alloc mem;
    CoreGraphics::TextureRelativeDimensions relativeDims;
    Ids::Id32 swapExtension;
    Ids::Id32 stencilExtension;
    Ids::Id32 sparseExtension;
    CoreGraphics::WindowId window; // Only valid if texture is window relative in size
};

struct VkTextureRuntimeInfo
{
    VkImageView view;
    CoreGraphics::TextureType type;
    uint32_t bind;
};

struct VkTextureMappingInfo
{
    VkBuffer buf;
    VkDeviceMemory mem;
    VkImageCopy region;
    uint32_t mapCount;
};

struct VkTextureSwapInfo
{
    Util::FixedArray<VkImage> swapimages;
    Util::FixedArray<VkImageView> swapviews;
};

enum
{
    Texture_RuntimeInfo,
    Texture_LoadInfo,
    Texture_MappingInfo,
};

/// we need a thread-safe allocator since it will be used by both the memory and stream pool
typedef Ids::IdAllocatorSafe<
    0xFFFF
    , VkTextureRuntimeInfo                   // runtime info (for binding)
    , VkTextureLoadInfo                      // loading info (mostly used during the load/unload phase)
    , VkTextureMappingInfo                   // used when image is mapped to memory
> VkTextureAllocator;
extern VkTextureAllocator textureAllocator;

enum
{
    TextureExtension_StencilInfo
    , TextureExtension_StencilBind
};
typedef Ids::IdAllocatorSafe<
    0xFFFF
    , CoreGraphics::TextureViewId
    , IndexT
> VkTextureStencilExtensionAllocator;
extern VkTextureStencilExtensionAllocator textureStencilExtensionAllocator;

enum
{
    TextureExtension_SwapInfo
};
typedef Ids::IdAllocatorSafe<
    0xFF
    , VkTextureSwapInfo
> VkTextureSwapExtensionAllocator;
extern VkTextureSwapExtensionAllocator textureSwapExtensionAllocator;

struct TextureSparsePageTable
{
    Util::FixedArray<Util::FixedArray<Util::Array<CoreGraphics::TextureSparsePage>>> pages;
    Util::FixedArray<Util::FixedArray<Util::Array<VkSparseImageMemoryBind>>> pageBindings;
    Util::FixedArray<Util::FixedArray<Util::FixedArray<uint32_t>>> bindCounts;
    VkMemoryRequirements memoryReqs;
};

enum
{
    TextureExtension_SparsePageTable,
    TextureExtension_SparseMemoryRequirements,
    TextureExtension_SparseOpaqueBinds,
    TextureExtension_SparsePendingBinds,
    TextureExtension_SparseOpaqueAllocs
};
typedef Ids::IdAllocatorSafe<
    0xFF
    , TextureSparsePageTable
    , VkSparseImageMemoryRequirements
    , Util::Array<VkSparseMemoryBind>
    , Util::Array<VkSparseImageMemoryBind>
    , Util::Array<CoreGraphics::Alloc>
> VkTextureSparseExtensionAllocator;
extern VkTextureSparseExtensionAllocator textureSparseExtensionAllocator;

/// get Vk image
const VkImage TextureGetVkImage(const CoreGraphics::TextureId id);
/// get vk image view
const VkImageView TextureGetVkImageView(const CoreGraphics::TextureId id);
/// get vk image view for stencil
const VkImageView TextureGetVkStencilImageView(const CoreGraphics::TextureId id);
/// get the device created the image
const VkDevice TextureGetVkDevice(const CoreGraphics::TextureId id);

} // namespace Vulkan
