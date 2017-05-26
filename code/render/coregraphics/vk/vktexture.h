#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan texture.

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
namespace Vulkan
{
class VkTexture : public Base::TextureBase
{
	__DeclareClass(VkTexture);
public:
	/// constructor
	VkTexture();
	/// destructor
	virtual ~VkTexture();

	/// unload the resource, or cancel the pending load
	virtual void Unload();
	/// map a texture mip level for CPU access
	bool Map(IndexT mipLevel, MapType mapType, MapInfo& outMapInfo);
	/// unmap texture after CPU access
	void Unmap(IndexT mipLevel);
	/// map a cube map face for CPU access
	bool MapCubeFace(CubeFace face, IndexT mipLevel, MapType mapType, MapInfo& outMapInfo);
	/// unmap cube map face after CPU access
	void UnmapCubeFace(CubeFace face, IndexT mipLevel);
	/// generates mipmaps
	void GenerateMipmaps();

	/// updates texture region
	void Update(void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip);
	/// updates entire texture
	void Update(void* data, SizeT dataSize, IndexT mip);
	/// updates texture cube face region
	void UpdateArray(void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer);
	/// updates texture cube face region
	void UpdateArray(void* data, SizeT dataSize, IndexT mip, IndexT layer);

	/// setup from an Vulkan 2D texture
	void SetupFromVkTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, CoreGraphics::PixelFormat::Code format, uint32_t numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
	/// setup from an Vulkan 2d multisample texture
	void SetupFromVkMultisampleTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, CoreGraphics::PixelFormat::Code format, uint32_t numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
	/// setup from an Vulkan texture cube
	void SetupFromVkCubeTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, CoreGraphics::PixelFormat::Code format, uint32_t numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
	/// setup from an Vulkan volume texture
	void SetupFromVkVolumeTexture(VkImage img, VkDeviceMemory mem, VkImageView imgView, uint32_t width, uint32_t height, uint32_t depth, CoreGraphics::PixelFormat::Code format, uint32_t numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
	/// setup from Vulkan back-buffer
	void SetupFromVkBackbuffer(VkImage img, uint32_t width, uint32_t height, uint32_t depth, CoreGraphics::PixelFormat::Code format);

	/// calculate the size of a texture given a certain mip, use face 0 when not accessing a cube or array texture
	void MipDimensions(IndexT mip, IndexT face, SizeT& width, SizeT& height, SizeT& depth);
	/// copy from one texture to another
	static void Copy(const Ptr<CoreGraphics::Texture>& from, const Ptr<CoreGraphics::Texture>& to, uint32_t width, uint32_t height, uint32_t depth, 
		uint32_t srcMip, uint32_t srcLayer, int32_t srcXOffset, int32_t srcYOffset, int32_t srcZOffset,
		uint32_t dstMip, uint32_t dstLayer, int32_t dstXOffset, int32_t dstYOffset, int32_t dstZOffset);

	/// get image
	const VkImage& GetVkImage() const;
	/// get image view
	const VkImageView& GetVkImageView() const;
	/// get image memory
	const VkDeviceMemory& GetVkMemory() const;

	/// get texture unique id
	const uint32_t GetVkId() const;
private:
	friend class VkRenderTexture;

	void* mappedData;
	uint32_t mapCount;
	VkImage img;
	VkImageView imgView;
	VkImageCreateInfo createInfo;
	VkDeviceMemory mem;

	uint32_t id;
	VkBuffer mappedImg;
	VkDeviceMemory mappedMem;
	VkImageCopy mappedBufferLayout;
	VkImageLayout currentLayout;
};

//------------------------------------------------------------------------------
/**
*/
inline const VkImage&
VkTexture::GetVkImage() const
{
	return this->img;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkImageView&
VkTexture::GetVkImageView() const
{
	return this->imgView;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkDeviceMemory&
VkTexture::GetVkMemory() const
{
	return this->mem;
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
VkTexture::GetVkId() const
{
	return this->id;
}

} // namespace Vulkan