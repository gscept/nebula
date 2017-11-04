#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan texture interface.

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
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/texturebase.h"
#include "coregraphics/pixelformat.h"
#include "resources/resourcepool.h"
#include "ids/idallocator.h"
namespace Vulkan
{
class VkTexture : public Base::TextureBase
{
public:
	/// constructor
	VkTexture();
	/// destructor
	virtual ~VkTexture();

	/// unload
	static void Unload(const VkDeviceMemory mem, const VkImage img, const VkImageView view);
	/// generates mipmaps
	static void GenerateMipmaps();

	/// map a texture mip level for CPU access
	static bool Map(const Resources::ResourceId id, IndexT mipLevel, Base::GpuResourceBase::MapType mapType, Base::TextureBase::MapInfo& outMapInfo);
	/// unmap texture after CPU access
	static void Unmap(const Resources::ResourceId id, IndexT mipLevel);
	/// map a cube map face for CPU access
	static bool MapCubeFace(const Resources::ResourceId id, Base::TextureBase::CubeFace face, IndexT mipLevel, Base::GpuResourceBase::MapType mapType, Base::TextureBase::MapInfo& outMapInfo);
	/// unmap cube map face after CPU access
	static void UnmapCubeFace(const Resources::ResourceId id, Base::TextureBase::CubeFace face, IndexT mipLevel);

	/// updates texture region
	static void Update(VkImage img, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip);
	/// updates entire texture
	static void Update(VkImage img, TextureBase::Dimensions dims, void* data, SizeT dataSize, IndexT mip);
	/// updates texture cube face region
	static void UpdateArray(VkImage img, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer);
	/// updates texture cube face region
	static void UpdateArray(VkImage img, TextureBase::Dimensions dims, void* data, SizeT dataSize, IndexT mip, IndexT layer);

	/// calculate the size of a texture given a certain mip, use face 0 when not accessing a cube or array texture
	void MipDimensions(IndexT mip, IndexT face, SizeT& width, SizeT& height, SizeT& depth);
	/// copy from one texture to another
	static void Copy(const VkImage from, const VkImage to, uint32_t width, uint32_t height, uint32_t depth,
		uint32_t srcMip, uint32_t srcLayer, int32_t srcXOffset, int32_t srcYOffset, int32_t srcZOffset,
		uint32_t dstMip, uint32_t dstLayer, int32_t dstXOffset, int32_t dstYOffset, int32_t dstZOffset);

private:
	friend class VkRenderTexture;
	friend class VkTexturePool;
	friend class VkMemoryTexturePool;
	friend class VkShaderPool;
	friend class VkShaderVariable;

	struct LoadInfo
	{
		VkImage img;
		VkDeviceMemory mem;
		TextureBase::Dimensions dims;
		uint32_t mips;
		CoreGraphics::PixelFormat::Code format;
		Base::GpuResourceBase::Usage usage;
		Base::GpuResourceBase::Access access;
		Base::GpuResourceBase::Syncing syncing;
	};
	struct RuntimeInfo
	{
		VkImageView view;
		TextureBase::Type type;
		uint32_t bind;
	};

	struct MappingInfo
	{
		VkBuffer buf;
		VkDeviceMemory mem;
		VkImageCopy region;
		uint32_t mapCount;
	};

	/// we need a thread-safe allocator since it will be used by both the memory and stream pool
	typedef Ids::IdAllocatorSafe<
		RuntimeInfo,						// 0 runtime info (for binding)
		LoadInfo,							// 1 loading info (mostly used during the load/unload phase)
		MappingInfo							// 2 used when image is mapped to memory
	> VkTextureAllocator;
	static VkTextureAllocator textureAllocator;
};

} // namespace Vulkan