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
#include "resources/resourcecache.h"
#include "ids/idallocator.h"
#include "vkmemory.h"
#include "coregraphics/load/glimltypes.h"

namespace IO
{
class Stream;
};

namespace Vulkan
{
struct VkTextureLoadInfo
{
    VkDevice dev;
    VkImage img;
    CoreGraphics::Alloc mem;
    CoreGraphics::TextureDimensions dims;
    CoreGraphics::TextureRelativeDimensions relativeDims;
    uint32_t mips;
    uint32_t layers;
    uint8_t samples;
    bool clear;
    union
    {
        Math::float4 clearColor;
        CoreGraphics::DepthStencilClear clearDepthStencil;
    };
    CoreGraphics::PixelFormat::Code format;
    CoreGraphics::TextureUsage texUsage;
    CoreGraphics::TextureId alias;
    CoreGraphics::ImageLayout defaultLayout;
    bool windowTexture : 1;                     // texture is meant to be a window back buffer
    bool windowRelative : 1;                    // size is a window relative percentage if true, other wise size is an absolute size
    bool bindless : 1;
    bool sparse : 1;                            // use sparse memory
    const void* texBuffer;                      // used when intially loading a texture from memory. Do not assume ownership of this pointer, this is just an intermediate.
    Ids::Id32 swapExtension;
    Ids::Id32 stencilExtension;
    Ids::Id32 sparseExtension;
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

struct VkTextureStreamInfo
{
    uint32_t lowestLod;
    void* mappedBuffer;
    uint bufferSize;
    uint32_t maxMips;
    Ptr<IO::Stream> stream;
    gliml::context ctx;
};

struct VkTextureWindowInfo
{
    CoreGraphics::WindowId window;
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
    Texture_WindowInfo,
    Texture_StreamInfo
};

/// we need a thread-safe allocator since it will be used by both the memory and stream pool
typedef Ids::IdAllocatorSafe<
    VkTextureRuntimeInfo,                   // runtime info (for binding)
    VkTextureLoadInfo,                      // loading info (mostly used during the load/unload phase)
    VkTextureMappingInfo,                   // used when image is mapped to memory
    VkTextureWindowInfo,
    VkTextureStreamInfo
> VkTextureAllocator;
extern VkTextureAllocator textureAllocator;

enum
{
    TextureExtension_StencilInfo
    , TextureExtension_StencilBind
};
typedef Ids::IdAllocatorSafe<
    CoreGraphics::TextureViewId
    , IndexT
> VkTextureStencilExtensionAllocator;
extern VkTextureStencilExtensionAllocator textureStencilExtensionAllocator;

enum
{
    TextureExtension_SwapInfo
};
typedef Ids::IdAllocatorSafe<
    VkTextureSwapInfo
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
    TextureSparsePageTable,
    VkSparseImageMemoryRequirements,
    Util::Array<VkSparseMemoryBind>,
    Util::Array<VkSparseImageMemoryBind>,
    Util::Array<CoreGraphics::Alloc>
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
