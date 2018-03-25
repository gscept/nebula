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
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcememorypool.h"
#include "coregraphics/pixelformat.h"
#include "vktexture.h"
#include "vkshaderserver.h"
#include "coregraphics/texture.h"
#include "coregraphics/gpubuffertypes.h"
#include <array>

namespace Vulkan
{
class VkMemoryTexturePool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkMemoryTexturePool);
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
	/// copy from one texture to another
	void Copy(const CoreGraphics::TextureId from, const CoreGraphics::TextureId to, SizeT width, SizeT height, SizeT depth,
		IndexT srcMip, IndexT srcLayer, SizeT srcXOffset, SizeT srcYOffset, SizeT srcZOffset,
		IndexT dstMip, IndexT dstLayer, SizeT dstXOffset, SizeT dstYOffset, SizeT dstZOffset);

	/// get texture dimensions
	CoreGraphics::TextureDimensions GetDimensions(const CoreGraphics::TextureId id);
	/// get texture pixel format
	CoreGraphics::PixelFormat::Code GetPixelFormat(const CoreGraphics::TextureId id);
	/// get texture type
	CoreGraphics::TextureType GetType(const CoreGraphics::TextureId id);
	/// get number of mips
	uint GetNumMips(const CoreGraphics::TextureId id);
private:
	friend class VkStreamTexturePool;
	__ImplementResourceAllocatorTypedSafe(textureAllocator, TextureIdType);
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkMemoryTexturePool::Unload(const Resources::ResourceId id)
{
	this->EnterGet();
	VkTextureLoadInfo& loadInfo = this->Get<1>(id);
	VkTextureRuntimeInfo& runtimeInfo = this->Get<0>(id);
	vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
	vkDestroyImage(loadInfo.dev, loadInfo.img, nullptr);
	vkDestroyImageView(loadInfo.dev, runtimeInfo.view, nullptr);
	VkShaderServer::Instance()->UnregisterTexture(runtimeInfo.bind, runtimeInfo.type);
	this->LeaveGet();
}

} // namespace Vulkan