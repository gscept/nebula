//------------------------------------------------------------------------------
// vkrendertexture.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkrendertexture.h"
#include "vktypes.h"
#include "vkrenderdevice.h"
#include "vkutilities.h"
#include "vkscheduler.h"
#include "coregraphics/window.h"
#include "coregraphics/displaydevice.h"

using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkRenderTexture, 'VKRT', Base::RenderTextureBase);
//------------------------------------------------------------------------------
/**
*/
VkRenderTexture::VkRenderTexture() :
	img(VK_NULL_HANDLE),
	mem(VK_NULL_HANDLE),
	view(VK_NULL_HANDLE)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkRenderTexture::~VkRenderTexture()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderTexture::Setup()
{
	RenderTextureBase::Setup();
	VkScheduler* scheduler = VkScheduler::Instance();

	// if this is a window texture, get the backbuffers from the render device
	if (this->windowTexture)
	{
		this->swapimages = this->window->backbuffers;
		VkClearColorValue clear = { 0, 0, 0, 0 };

		VkImageSubresourceRange subres;
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;

		// clear textures
		IndexT i;
		for (i = 0; i < this->swapimages.Size(); i++)
		{
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->swapimages[i], subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
			scheduler->PushImageColorClear(this->swapimages[i], VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->swapimages[i], subres, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
		}

		// setup texture
		this->texture->SetupFromVkBackbuffer(this->swapimages[0], this->width, this->height, this->depth, this->format);
	}
	else
	{
		VkSampleCountFlagBits sampleCount = this->msaaEnabled ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;

		VkExtent3D extents;
		extents.width = this->width;
		extents.height = this->height;
		extents.depth = this->depth;

		VkImageViewType viewType;
		switch (this->type)
		{
		case Texture::Texture2D:
			viewType = VK_IMAGE_VIEW_TYPE_2D;
			break;
		case Texture::Texture2DArray:
			viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			break;
		case Texture::TextureCube:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			break;
		case Texture::TextureCubeArray:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			break;
		}

        VkFormat fmt = VkTypes::AsVkFramebufferFormat(this->format);
        if (this->usage == ColorAttachment)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(VkRenderDevice::physicalDev, fmt, &formatProps);
            n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT &&
                     formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT &&
                     formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        }
		VkImageUsageFlags usageFlags = this->usage == ColorAttachment ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		VkImageCreateInfo imgInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			NULL,
			0,
			VK_IMAGE_TYPE_2D,   // if we have a cube map, it's just 2D * 6
			fmt,
			extents,
			1,
			(uint32_t)this->layers,
			sampleCount,
			VK_IMAGE_TILING_OPTIMAL,
			usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			NULL,
			VK_IMAGE_LAYOUT_UNDEFINED
		};

		// create image for rendering
		VkResult res = vkCreateImage(VkRenderDevice::dev, &imgInfo, NULL, &this->img);
		n_assert(res == VK_SUCCESS);

		// allocate buffer backing and bind to image
		uint32_t size;
		VkUtilities::AllocateImageMemory(this->img, this->mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);
		vkBindImageMemory(VkRenderDevice::dev, this->img, this->mem, 0);

		VkImageAspectFlags aspect = this->usage == ColorAttachment ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		VkImageSubresourceRange subres;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;
		subres.aspectMask = aspect;
 		VkImageViewCreateInfo viewInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0,
			this->img,
			viewType,
			VkTypes::AsVkFramebufferFormat(this->format),
			VkTypes::AsVkMapping(this->format),
			subres
		};

		res = vkCreateImageView(VkRenderDevice::dev, &viewInfo, NULL, &this->view);
		n_assert(res == VK_SUCCESS);

		if (this->usage == ColorAttachment)
		{
			// clear image and transition layout
			VkClearColorValue clear = { 0, 0, 0, 0 };
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
			scheduler->PushImageColorClear(this->img, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}
		else
		{
			// clear image and transition layout
			VkClearDepthStencilValue clear = { 1, 0 };
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
			scheduler->PushImageDepthStencilClear(this->img, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
		}

		// setup underlying texture
		if (sampleCount > 1)
		{
			this->texture->SetupFromVkMultisampleTexture(this->img, this->mem, this->view, this->width, this->height, this->format, 0, true, true);
		}
		else
		{
			this->texture->SetupFromVkTexture(this->img, this->mem, this->view, this->width, this->height, this->format, 0, true, true);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderTexture::Resize()
{
	RenderTextureBase::Resize();

	if (!this->windowTexture)
	{
		// dealloc all resources
		vkDestroyImageView(VkRenderDevice::dev, this->view, nullptr);
		vkDestroyImage(VkRenderDevice::dev, this->img, nullptr);
		vkFreeMemory(VkRenderDevice::dev, this->mem, nullptr);

		VkScheduler* scheduler = VkScheduler::Instance();
		VkSampleCountFlagBits sampleCount = this->msaaEnabled ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;

		VkExtent3D extents;
		extents.width = this->width;
		extents.height = this->height;
		extents.depth = this->depth;

		VkImageViewType viewType;
		switch (this->type)
		{
		case Texture::Texture2D:
			viewType = VK_IMAGE_VIEW_TYPE_2D;
			break;
		case Texture::Texture2DArray:
			viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			break;
		case Texture::TextureCube:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			break;
		case Texture::TextureCubeArray:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			break;
		}

		VkFormat fmt = VkTypes::AsVkFramebufferFormat(this->format);
		if (this->usage == ColorAttachment)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(VkRenderDevice::physicalDev, fmt, &formatProps);
			n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT &&
				formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT &&
				formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		}
		VkImageUsageFlags usageFlags = this->usage == ColorAttachment ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		VkImageCreateInfo imgInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			NULL,
			0,
			VK_IMAGE_TYPE_2D,   // if we have a cube map, it's just 2D * 6
			fmt,
			extents,
			1,
			(uint32_t)this->layers,
			sampleCount,
			VK_IMAGE_TILING_OPTIMAL,
			usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			NULL,
			VK_IMAGE_LAYOUT_UNDEFINED
		};

		// create image for rendering
		VkResult res = vkCreateImage(VkRenderDevice::dev, &imgInfo, NULL, &this->img);
		n_assert(res == VK_SUCCESS);

		// allocate buffer backing and bind to image
		uint32_t size;
		VkUtilities::AllocateImageMemory(this->img, this->mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);
		vkBindImageMemory(VkRenderDevice::dev, this->img, this->mem, 0);

		VkImageAspectFlags aspect = this->usage == ColorAttachment ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		VkImageSubresourceRange subres;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;
		subres.aspectMask = aspect;
		VkImageViewCreateInfo viewInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0,
			this->img,
			viewType,
			VkTypes::AsVkFramebufferFormat(this->format),
			VkTypes::AsVkMapping(this->format),
			subres
		};

		res = vkCreateImageView(VkRenderDevice::dev, &viewInfo, NULL, &this->view);
		n_assert(res == VK_SUCCESS);

		// clear
		if (this->usage == ColorAttachment)
		{
			// clear image and transition layout
			VkClearColorValue clear = { 0, 0, 0, 0 };
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
			scheduler->PushImageColorClear(this->img, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}
		else
		{
			// clear image and transition layout
			VkClearDepthStencilValue clear = { 1, 0 };
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
			scheduler->PushImageDepthStencilClear(this->img, VkDeferredCommand::Graphics, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
			scheduler->PushImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, subres, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
		}

		// setup underlying texture
		if (sampleCount > 1)
		{
			this->texture->SetupFromVkMultisampleTexture(this->img, this->mem, this->view, this->width, this->height, this->format, 0, true, true);
		}
		else
		{
			this->texture->SetupFromVkTexture(this->img, this->mem, this->view, this->width, this->height, this->format, 0, true, true);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderTexture::GenerateMipChain()
{
	Base::RenderTextureBase::GenerateMipChain();
	uint32_t numMips = this->texture->GetNumMipLevels();
	uint32_t i;
	for (i = 1; i < numMips; i++)
	{
		this->GenerateMipHelper(0, i, this);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderTexture::GenerateMipChain(IndexT from)
{
	Base::RenderTextureBase::GenerateMipChain(from);
	uint32_t numMips = this->texture->GetNumMipLevels() - from;
	uint32_t i;
	for (i = from + 1; i < numMips; i++)
	{
		this->GenerateMipHelper(from, i, this);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderTexture::GenerateMipChain(IndexT from, IndexT to)
{
	Base::RenderTextureBase::GenerateMipChain(from, to);
	IndexT i;
	for (i = from+1; i < to; i++)
	{
		this->GenerateMipHelper(from, i, this);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderTexture::Blit(IndexT fromMip, IndexT toMip, const Ptr<CoreGraphics::RenderTexture>& target)
{
	Base::RenderTextureBase::Blit(fromMip, toMip);
	this->GenerateMipHelper(fromMip, toMip, target == nullptr ? this : target);
}

//------------------------------------------------------------------------------
/**
	Internal helper function to generate mips, will assert that the texture is not within a pass
*/
void
VkRenderTexture::GenerateMipHelper(IndexT from, IndexT to, const Ptr<VkRenderTexture>& target)
{
	n_assert(!this->isInPass);
	n_assert(this->format == target->format);
	VkRenderDevice* dev = VkRenderDevice::Instance();

	// setup from-region
	int32_t fromMipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(this->width / Math::n_pow(2, (float)from)));
	int32_t fromMipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(this->height / Math::n_pow(2, (float)from)));
	Math::rectangle<int> fromRegion;
	fromRegion.left = 0;
	fromRegion.top = 0;
	fromRegion.right = fromMipWidth;
	fromRegion.bottom = fromMipHeight;

	VkImageSubresourceRange fromSubres;
	fromSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	fromSubres.baseArrayLayer = 0;
	fromSubres.baseMipLevel = from;
	fromSubres.layerCount = 1;
	fromSubres.levelCount = 1;

	// transition source to blit source
	VkUtilities::ImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, fromSubres, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

	VkImageSubresourceRange toSubres;
	toSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	toSubres.baseArrayLayer = 0;
	toSubres.baseMipLevel = to;
	toSubres.layerCount = 1;
	toSubres.levelCount = 1;

	int32_t toMipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(target->width / Math::n_pow(2, (float)to)));
	int32_t toMipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(target->height / Math::n_pow(2, (float)to)));
	Math::rectangle<int> toRegion;
	toRegion.left = 0;
	toRegion.top = 0;
	toRegion.right = toMipWidth;
	toRegion.bottom = toMipHeight;
	
	// create smart pointer to self
	Ptr<CoreGraphics::RenderTexture> ptr = Ptr<VkRenderTexture>(this).downcast<CoreGraphics::RenderTexture>();

	// transition the texture to destination, blit, and transition it back
	VkUtilities::ImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(target->img, toSubres, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
	dev->Blit(ptr, fromRegion, from, target.downcast<CoreGraphics::RenderTexture>(), toRegion, to);
	VkUtilities::ImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(target->img, toSubres, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	// transition source out from being blit source
	VkUtilities::ImageLayoutTransition(VkDeferredCommand::Graphics, VkUtilities::ImageMemoryBarrier(this->img, fromSubres, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
}

//------------------------------------------------------------------------------
/**
*/
void
VkRenderTexture::SwapBuffers()
{
	RenderTextureBase::SwapBuffers();
	Ptr<CoreGraphics::Window> wnd = this->window;
	VkRenderDevice* dev = VkRenderDevice::Instance();
	VkResult res = vkAcquireNextImageKHR(dev->dev, wnd->swapchain, UINT64_MAX, wnd->displaySemaphore, VK_NULL_HANDLE, &wnd->currentBackbuffer);
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// this means our swapchain needs a resize!
	}
	else
	{
		n_assert(res == VK_SUCCESS);
	}

	// set image and update texture
	this->img = this->swapimages[wnd->currentBackbuffer];
	this->texture->img = this->img;
}


} // namespace Vulkan