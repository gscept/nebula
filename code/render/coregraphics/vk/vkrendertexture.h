#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan implementation of a render texture
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/rendertexturebase.h"
namespace CoreGraphics
{
class RenderTexture;
}

namespace Vulkan
{
class VkRenderTexture : public Base::RenderTextureBase
{
	__DeclareClass(VkRenderTexture);
public:
	/// constructor
	VkRenderTexture();
	/// destructor
	virtual ~VkRenderTexture();

	/// setup render texture
	void Setup();
	/// resize render texture, retaining the same texture object
	void Resize();
	
	/// generate mip chain (from 0 to number of mips)
	void GenerateMipChain();
	/// generate mip chain from offset (from N to number of mips)
	void GenerateMipChain(IndexT from);
	/// generate segment of mip chain
	void GenerateMipChain(IndexT from, IndexT to);
	/// generate mip from one mip level to another
	void Blit(IndexT from, IndexT to, const Ptr<CoreGraphics::RenderTexture>& target = nullptr);

	/// swap buffers, only valid if this is a window texture
	void SwapBuffers();

	/// get image
	const VkImage& GetVkImage() const;
	/// get image view
	const VkImageView& GetVkImageView() const;
	/// get memory
	const VkDeviceMemory& GetVkMemory() const;
private:
	
	/// generate mip maps internally from index to another
	void GenerateMipHelper(IndexT from, IndexT to, const Ptr<VkRenderTexture>& target);
	VkImage img;
	VkImageView view;
	VkDeviceMemory mem;

	Util::FixedArray<VkImage> swapimages;
};

//------------------------------------------------------------------------------
/**
*/
inline const VkImage&
VkRenderTexture::GetVkImage() const
{
	return this->img;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkImageView&
VkRenderTexture::GetVkImageView() const
{
	return this->view;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkDeviceMemory&
VkRenderTexture::GetVkMemory() const
{
	return this->mem;
}

} // namespace Vulkan