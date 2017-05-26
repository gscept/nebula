#pragma once
//------------------------------------------------------------------------------
/**
	Implements a read/write image in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/shaderreadwritetexturebase.h"
namespace Vulkan
{
class VkShaderImage : public Base::ShaderReadWriteTextureBase
{
	__DeclareClass(VkShaderImage);
public:
	/// constructor
	VkShaderImage();
	/// destructor
	virtual ~VkShaderImage();

	/// setup texture with fixed size
	void Setup(const SizeT width, const SizeT height, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id);

	/// resize texture
	void Resize();
	/// clear texture
	void Clear(const Math::float4& clearColor);

	/// get image
	const VkImage& GetVkImage() const;
	/// get image view
	const VkImageView& GetVkImageView() const;
	/// get image memory
	const VkDeviceMemory& GetVkMemory() const;

private:

	VkImage img;
	VkDeviceMemory mem;
	VkImageView view;
};


//------------------------------------------------------------------------------
/**
*/
inline const VkImage&
VkShaderImage::GetVkImage() const
{
	return this->img;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkImageView&
VkShaderImage::GetVkImageView() const
{
	return this->view;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkDeviceMemory&
VkShaderImage::GetVkMemory() const
{
	return this->mem;
}

} // namespace Vulkan