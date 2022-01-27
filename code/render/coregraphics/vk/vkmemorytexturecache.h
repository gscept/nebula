#pragma once
//------------------------------------------------------------------------------
/**
    Handles memory-loaded Vulkan textures.

    TODO: Mapping creates a new buffer every time. This is broken,
    since mapping more than once should just return the same buffer.

    Implements a shared texture resource pool for both the memory and stream pool.
    It's thread safe, so unnecessary loads and maps might result in thread synchronizations.

    Mapping a Vulkan buffer requires an explicit buffer backing, which is created when mapping.
    This buffer is then what you can read/write to, and when you unmap it the data will be flushed back, and the buffer deleted.

    Mapping is slow for this reason, so use with caution. We can not apply a persistently mapped buffer either, since texture data is
    non-linear and can have any type of implementation specific way of treating data as pixels.

    Also be careful when to call Unload, since we might have in-flight texture updates through Update, operating on a texture which may
    have been deleted.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcememorycache.h"
#include "coregraphics/pixelformat.h"
#include "vktexture.h"
#include "vkshaderserver.h"
#include "coregraphics/texture.h"
#include "coregraphics/gpubuffertypes.h"
#include <array>

namespace Vulkan
{
class VkMemoryTextureCache : public Resources::ResourceMemoryCache
{
    __DeclareClass(VkMemoryTextureCache);
public:

    struct VkMemoryTextureInfo
    {
        const void* buffer;
        CoreGraphics::PixelFormat::Code format;
        SizeT width, height;
    };

    /// update resource
    LoadStatus LoadFromMemory(const Resources::ResourceId id, const void* info) override;
    /// unload resource
    void Unload(const Resources::ResourceId id) override;
    /// reload resource. This will unload and then reload with the same info as when previously loaded
    void Reload(const Resources::ResourceId id);

    /// generates mipmaps
    void GenerateMipmaps(const CoreGraphics::TextureId id);

    /// map a texture mip level for CPU access
    bool Map(const CoreGraphics::TextureId id, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo& outMapInfo);
    /// unmap texture after CPU access
    void Unmap(const CoreGraphics::TextureId id, IndexT mipLevel);
    /// map a cube map face for CPU access
    bool MapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo& outMapInfo);
    /// unmap cube map face after CPU access
    void UnmapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel);

    /// updates texture region
    void Update(const CoreGraphics::TextureId id, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip);
    /// updates entire texture
    void Update(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void* data, SizeT dataSize, IndexT mip);
    /// updates texture cube face region
    void UpdateArray(const CoreGraphics::TextureId id, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer);
    /// updates texture cube face region
    void UpdateArray(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void* data, SizeT dataSize, IndexT mip, IndexT layer);

    /// calculate the size of a texture given a certain mip, use face 0 when not accessing a cube or array texture
    void MipDimensions(IndexT mip, IndexT face, SizeT& width, SizeT& height, SizeT& depth);

    /// update texture from data stream
    void Update(const CoreGraphics::TextureId id, const Math::rectangle<int>& region, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub);

    /// clear texture with color
    void ClearColor(const CoreGraphics::TextureId id, Math::vec4 color, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres, const CoreGraphics::SubmissionContextId sub);
    /// clear texture with depth-stencil
    void ClearDepthStencil(const CoreGraphics::TextureId id, float depth, uint stencil, const CoreGraphics::ImageLayout layout, const CoreGraphics::ImageSubresourceInfo& subres, const CoreGraphics::SubmissionContextId sub);

    /// get texture dimensions
    CoreGraphics::TextureDimensions GetDimensions(const CoreGraphics::TextureId id);
    /// get texture relative dimensions
    CoreGraphics::TextureRelativeDimensions GetRelativeDimensions(const CoreGraphics::TextureId id);
    /// get texture pixel format
    CoreGraphics::PixelFormat::Code GetPixelFormat(const CoreGraphics::TextureId id);
    /// get texture type
    CoreGraphics::TextureType GetType(const CoreGraphics::TextureId id);
    /// get texture alias
    CoreGraphics::TextureId GetAlias(const CoreGraphics::TextureId id);
    /// get texture usage
    CoreGraphics::TextureUsage GetUsageBits(const CoreGraphics::TextureId id);
    /// get number of mips
    SizeT GetNumMips(const CoreGraphics::TextureId id);
    /// get number of layers
    SizeT GetNumLayers(const CoreGraphics::TextureId id);
    /// get number of samples
    SizeT GetNumSamples(const CoreGraphics::TextureId id);
    /// get bindless handle
    uint GetBindlessHandle(const CoreGraphics::TextureId id);
    /// get bindless handle
    uint GetStencilBindlessHandle(const CoreGraphics::TextureId id);
    /// get default layout
    CoreGraphics::ImageLayout GetDefaultLayout(const CoreGraphics::TextureId id);

    /// get the texture page size, which is constant for the whole texture
    CoreGraphics::TextureSparsePageSize SparseGetPageSize(const CoreGraphics::TextureId id);
    /// get the page index at a given coordinate
    IndexT SparseGetPageIndex(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT x, IndexT y, IndexT z);
    /// get texture page
    const CoreGraphics::TextureSparsePage& SparseGetPage(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex);
    /// get the number of pages for a given layer and mip
    SizeT SparseGetNumPages(const CoreGraphics::TextureId id, IndexT layer, IndexT mip);
    /// get max mip
    IndexT SparseGetMaxMip(const CoreGraphics::TextureId id);

    /// evict a page
    void SparseEvict(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex);
    /// make a page resident
    void SparseMakeResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip, IndexT pageIndex);
    /// evict a mip
    void SparseEvictMip(const CoreGraphics::TextureId id, IndexT layer, IndexT mip);
    /// make mip resident
    void SparseMakeMipResident(const CoreGraphics::TextureId id, IndexT layer, IndexT mip);
    /// commit texture sparse page updates
    void SparseCommitChanges(const CoreGraphics::TextureId id);

    /// update a region of the sparse texture, make sure to insert barriers before doing this though
    void SparseUpdate(const CoreGraphics::TextureId id, const Math::rectangle<uint>& region, IndexT mip, IndexT layer, const CoreGraphics::TextureId source, const CoreGraphics::SubmissionContextId sub);
    /// update a region of the sparse texture, make sure to insert barriers before doing this though
    void SparseUpdate(const CoreGraphics::TextureId id, const Math::rectangle<uint>& region, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub);
    /// update a whole mip
    void SparseUpdate(const CoreGraphics::TextureId id, IndexT mip, IndexT layer, char* buf, const CoreGraphics::SubmissionContextId sub);

    /// swap buffers for texture
    IndexT SwapBuffers(const CoreGraphics::TextureId id);
private:
    /// setup textures from load info
    bool Setup(const Resources::ResourceId id);

    friend void CoreGraphics::DelayedDeleteTexture(const TextureId id);

    friend class VkStreamTextureCache;
    __ImplementResourceAllocatorTypedSafe(textureAllocator, CoreGraphics::TextureIdType);
};

} // namespace Vulkan
