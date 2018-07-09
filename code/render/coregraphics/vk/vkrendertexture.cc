//------------------------------------------------------------------------------
// vkrendertexture.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkrendertexture.h"
#include "vktypes.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
#include "vkscheduler.h"
#include "coregraphics/window.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/config.h"
#include "coregraphics/glfw/glfwwindow.h"
#include "coregraphics/shaderserver.h"

namespace Vulkan
{

VkRenderTextureAllocator renderTextureAllocator;

}

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
RenderTextureId
CreateRenderTexture(const RenderTextureCreateInfo& info)
{
	Ids::Id32 id = renderTextureAllocator.AllocObject();
	RenderTextureInfo adjustedInfo = RenderTextureInfoSetupHelper(info);
	VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id);
	VkRenderTextureRuntimeInfo& runtimeInfo = renderTextureAllocator.Get<1>(id);
	VkRenderTextureMappingInfo& mapInfo = renderTextureAllocator.Get<2>(id);
	VkRenderTextureWindowInfo& swapInfo = renderTextureAllocator.Get<3>(id);
	ImageLayout& layout = renderTextureAllocator.Get<4>(id);

	RenderTextureId rtId;
	rtId.id24 = id;
	rtId.id8 = RenderTextureIdType;

	// set map to 0
	mapInfo.mapCount = 0;
	
	// setup dimensions
	loadInfo.dims.width = adjustedInfo.width;
	loadInfo.dims.height = adjustedInfo.height;
	loadInfo.dims.depth = adjustedInfo.depth;
	loadInfo.widthScale = adjustedInfo.widthScale;
	loadInfo.heightScale = adjustedInfo.heightScale;
	loadInfo.depthScale = adjustedInfo.depthScale;
	loadInfo.mips = adjustedInfo.mips;
	loadInfo.layers = adjustedInfo.layers;
	loadInfo.format = adjustedInfo.format;
	loadInfo.relativeSize = adjustedInfo.relativeSize;
	loadInfo.msaa = adjustedInfo.msaa;
	loadInfo.dev = Vulkan::GetCurrentDevice();
	runtimeInfo.inpass = false;
	runtimeInfo.type = adjustedInfo.type;
	runtimeInfo.bind = -1;

	VkScheduler* scheduler = VkScheduler::Instance();

	// if this is a window texture, get the backbuffers from the render device
	if (adjustedInfo.isWindow)
	{
		VkBackbufferInfo& backbufferInfo = CoreGraphics::glfwWindowAllocator.Get<GLFWBackbufferField>(adjustedInfo.window.id24);
		swapInfo.swapimages = backbufferInfo.backbuffers;
		swapInfo.swapviews = backbufferInfo.backbufferViews;
		VkClearColorValue clear = { 0, 0, 0, 0 };

		VkImageSubresourceRange subres;
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;

		// clear textures
		IndexT i;
		for (i = 0; i < swapInfo.swapimages.Size(); i++)
		{
			scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Host, CoreGraphics::BarrierDependency::Transfer, VkUtilities::ImageMemoryBarrier(swapInfo.swapimages[i], subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
			scheduler->PushImageColorClear(swapInfo.swapimages[i], GraphicsQueueType, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, subres);
			scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Transfer, CoreGraphics::BarrierDependency::PassOutput, VkUtilities::ImageMemoryBarrier(swapInfo.swapimages[i], subres, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
		}
		
		n_assert(adjustedInfo.type == Texture2D);
		n_assert(adjustedInfo.mips == 1);
		n_assert(!adjustedInfo.msaa);
		
		layout = ImageLayout::Present;
		loadInfo.img = swapInfo.swapimages[0];
		loadInfo.isWindow = true;
		loadInfo.mem = VK_NULL_HANDLE;
		loadInfo.window = adjustedInfo.window;
		runtimeInfo.type = Texture2D;
		runtimeInfo.view = backbufferInfo.backbufferViews[0];
	}
	else
	{
		VkSampleCountFlagBits sampleCount = adjustedInfo.msaa ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;

		VkExtent3D extents;
		extents.width = adjustedInfo.width;
		extents.height = adjustedInfo.height;
		extents.depth = adjustedInfo.depth;

		VkImageViewType viewType;
		switch (adjustedInfo.type)
		{
		case Texture2D:
			viewType = VK_IMAGE_VIEW_TYPE_2D;
			break;
		case Texture2DArray:
			viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			break;
		case TextureCube:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			break;
		case TextureCubeArray:
			viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			break;
		}

		VkFormat fmt = VkTypes::AsVkFramebufferFormat(adjustedInfo.format);
		if (adjustedInfo.usage == ColorAttachment)
		{
			VkFormatProperties formatProps;
			vkGetPhysicalDeviceFormatProperties(Vulkan::GetCurrentPhysicalDevice(), fmt, &formatProps);
			n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT &&
				formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT &&
				formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		}
		VkImageUsageFlags usageFlags = adjustedInfo.usage == ColorAttachment ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		VkImageCreateInfo imgInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			NULL,
			0,
			VK_IMAGE_TYPE_2D,   // if we have a cube map, it's just 2D * 6
			fmt,
			extents,
			1,
			loadInfo.layers,
			sampleCount,
			VK_IMAGE_TILING_OPTIMAL,
			usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			NULL,
			VK_IMAGE_LAYOUT_UNDEFINED
		};

		// create image for rendering
		VkResult res = vkCreateImage(loadInfo.dev, &imgInfo, NULL, &loadInfo.img);
		n_assert(res == VK_SUCCESS);

		// allocate buffer backing and bind to image
		uint32_t size;
		VkUtilities::AllocateImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);
		vkBindImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, 0);

		VkImageAspectFlags aspect = adjustedInfo.usage == ColorAttachment ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
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
			loadInfo.img,
			viewType,
			VkTypes::AsVkFramebufferFormat(adjustedInfo.format),
			VkTypes::AsVkMapping(adjustedInfo.format),
			subres
		};

		res = vkCreateImageView(loadInfo.dev, &viewInfo, NULL, &runtimeInfo.view);
		n_assert(res == VK_SUCCESS);

		if (adjustedInfo.usage == ColorAttachment)
		{
			// clear image and transition layout
			VkClearColorValue clear = { 0, 0, 0, 0 };
			scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Host, CoreGraphics::BarrierDependency::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
			scheduler->PushImageColorClear(loadInfo.img, GraphicsQueueType, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, subres);
			scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Transfer, CoreGraphics::BarrierDependency::PassOutput, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

			// create binding
			layout = ImageLayout::ShaderRead;
			runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(rtId, false, runtimeInfo.type);

		}
		else
		{
			// clear image and transition layout
			VkClearDepthStencilValue clear = { 1, 0 };
			scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Host, CoreGraphics::BarrierDependency::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
			scheduler->PushImageDepthStencilClear(loadInfo.img, GraphicsQueueType, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, subres);
			scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Transfer, CoreGraphics::BarrierDependency::LateDepth, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));

			layout = ImageLayout::DepthStencilRead;
			//runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(rtId, true, runtimeInfo.type);
		}
	}
	return rtId;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyRenderTexture(const RenderTextureId id)
{
	VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	VkRenderTextureRuntimeInfo& runtimeInfo = renderTextureAllocator.Get<1>(id.id24);
	VkRenderTextureWindowInfo& swapInfo = renderTextureAllocator.Get<3>(id.id24);

	if (runtimeInfo.bind != -1)
		VkShaderServer::Instance()->UnregisterTexture(runtimeInfo.bind, runtimeInfo.type);

	if (loadInfo.isWindow)
	{
		swapInfo.swapimages.Clear();
		swapInfo.swapviews.Clear();
	}
	else
	{

		vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
		vkDestroyImage(loadInfo.dev, loadInfo.img, nullptr);		
		vkDestroyImageView(loadInfo.dev, runtimeInfo.view, nullptr);
	}
	renderTextureAllocator.DeallocObject(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureResize(const RenderTextureId id, const RenderTextureResizeInfo& info)
{
	RenderTextureInfo adjustedInfo = RenderTextureInfoResizeHelper(info);
	VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	VkRenderTextureRuntimeInfo& runtimeInfo = renderTextureAllocator.Get<1>(id.id24);
	VkRenderTextureMappingInfo& mapInfo = renderTextureAllocator.Get<2>(id.id24);
	n_assert(mapInfo.mapCount == 0);

	VkScheduler* scheduler = VkScheduler::Instance();
	
	VkSampleCountFlagBits sampleCount = adjustedInfo.msaa ? VK_SAMPLE_COUNT_16_BIT : VK_SAMPLE_COUNT_1_BIT;

	VkExtent3D extents;
	extents.width = adjustedInfo.width;
	extents.height = adjustedInfo.height;
	extents.depth = adjustedInfo.depth;

	VkImageViewType viewType;
	switch (adjustedInfo.type)
	{
	case Texture2D:
		viewType = VK_IMAGE_VIEW_TYPE_2D;
		break;
	case Texture2DArray:
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		break;
	case TextureCube:
		viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		break;
	case TextureCubeArray:
		viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		break;
	}

	VkFormat fmt = VkTypes::AsVkFramebufferFormat(adjustedInfo.format);
	if (adjustedInfo.usage == ColorAttachment)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(Vulkan::GetCurrentPhysicalDevice(), fmt, &formatProps);
		n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT &&
			formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT &&
			formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
	}
	VkImageUsageFlags usageFlags = adjustedInfo.usage == ColorAttachment ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkImageCreateInfo imgInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		0,
		VK_IMAGE_TYPE_2D,   // if we have a cube map, it's just 2D * 6
		fmt,
		extents,
		1,
		loadInfo.layers,
		sampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		usageFlags | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	// create image for rendering
	VkResult res = vkCreateImage(loadInfo.dev, &imgInfo, NULL, &loadInfo.img);
	n_assert(res == VK_SUCCESS);

	// allocate buffer backing and bind to image
	uint32_t size;
	VkUtilities::AllocateImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);
	vkBindImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, 0);

	VkImageAspectFlags aspect = adjustedInfo.usage == ColorAttachment ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
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
		loadInfo.img,
		viewType,
		VkTypes::AsVkFramebufferFormat(adjustedInfo.format),
		VkTypes::AsVkMapping(adjustedInfo.format),
		subres
	};

	res = vkCreateImageView(loadInfo.dev, &viewInfo, NULL, &runtimeInfo.view);
	n_assert(res == VK_SUCCESS);

	if (adjustedInfo.usage == ColorAttachment)
	{
		// clear image and transition layout
		VkClearColorValue clear = { 0, 0, 0, 0 };
		scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Host, CoreGraphics::BarrierDependency::NoDependencies, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
		scheduler->PushImageColorClear(loadInfo.img, GraphicsQueueType, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
		scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::PixelShader, CoreGraphics::BarrierDependency::AllGraphicsShaders, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}
	else
	{
		// clear image and transition layout
		VkClearDepthStencilValue clear = { 1, 0 };
		scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::Host, CoreGraphics::BarrierDependency::NoDependencies, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL));
		scheduler->PushImageDepthStencilClear(loadInfo.img, GraphicsQueueType, VK_IMAGE_LAYOUT_GENERAL, clear, subres);
		scheduler->PushImageLayoutTransition(GraphicsQueueType, CoreGraphics::BarrierDependency::NoDependencies, CoreGraphics::BarrierDependency::AllGraphicsShaders, VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureSwapBuffers(const CoreGraphics::RenderTextureId id)
{
	VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	VkRenderTextureRuntimeInfo& runtimeInfo = renderTextureAllocator.Get<1>(id.id24);
	n_assert(loadInfo.isWindow);
	VkWindowSwapInfo& swapInfo = CoreGraphics::glfwWindowAllocator.Get<5>(loadInfo.window.id24);
	VkResult res = vkAcquireNextImageKHR(Vulkan::GetCurrentDevice(), swapInfo.swapchain, UINT64_MAX, swapInfo.displaySemaphore, VK_NULL_HANDLE, &swapInfo.currentBackbuffer);
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// this means our swapchain needs a resize!
	}
	else
	{
		n_assert(res == VK_SUCCESS);
	}

	VkRenderTextureWindowInfo& wnd = renderTextureAllocator.Get<3>(id.id24);
	// set image and update texture
	loadInfo.img = wnd.swapimages[swapInfo.currentBackbuffer];
	runtimeInfo.view = wnd.swapviews[swapInfo.currentBackbuffer];
	//this->texture->img = this->img;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureWindowResized(const RenderTextureId id)
{
	const VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	RenderTextureResizeInfo info;
	info.width = loadInfo.dims.width;
	info.widthScale = loadInfo.widthScale;
	info.height = loadInfo.dims.height;
	info.heightScale = loadInfo.heightScale;
	info.depth = loadInfo.dims.depth;
	info.depthScale = loadInfo.depthScale;
	RenderTextureResize(id, info);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::TextureDimensions
RenderTextureGetDimensions(const RenderTextureId id)
{
	return renderTextureAllocator.Get<0>(id.id24).dims;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::PixelFormat::Code
RenderTextureGetPixelFormat(const RenderTextureId id)
{
	return renderTextureAllocator.Get<0>(id.id24).format;
}

//------------------------------------------------------------------------------
/**
*/
const bool
RenderTextureGetMSAA(const RenderTextureId id)
{
	return renderTextureAllocator.Get<0>(id.id24).msaa;
}

//------------------------------------------------------------------------------
/**
*/
const ImageLayout
RenderTextureGetLayout(const RenderTextureId id)
{
	return renderTextureAllocator.Get<4>(id.id24);
}

} // namespace CoreGraphics

namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureGenerateMipChain(const CoreGraphics::RenderTextureId id)
{
	const VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	uint32_t numMips = loadInfo.mips;
	uint32_t i;
	for (i = 1; i < numMips; i++)
	{
		RenderTextureGenerateMipHelper(id, 0, id, i);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureGenerateMipChain(const CoreGraphics::RenderTextureId id, IndexT from)
{
	const VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	n_assert(loadInfo.mips > (uint32_t)from);
	uint32_t numMips = loadInfo.mips - from;
	uint32_t i;
	for (i = from + 1; i < numMips; i++)
	{
		RenderTextureGenerateMipHelper(id, from, id, i);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureGenerateMipChain(const CoreGraphics::RenderTextureId id, IndexT from, IndexT to)
{
	const VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	n_assert(loadInfo.mips > (uint32_t)from && loadInfo.mips > (uint32_t)to);
	IndexT i;
	for (i = from+1; i < to; i++)
	{
		RenderTextureGenerateMipHelper(id, from, id, i);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
RenderTextureBlit(const CoreGraphics::RenderTextureId id, IndexT fromMip, IndexT toMip, const CoreGraphics::RenderTextureId target)
{
	const VkRenderTextureLoadInfo& loadInfo = renderTextureAllocator.Get<0>(id.id24);
	n_assert(loadInfo.mips > (uint32_t)fromMip && loadInfo.mips > (uint32_t)toMip);
	RenderTextureGenerateMipHelper(id, fromMip, target == CoreGraphics::RenderTextureId::Invalid() ? id : target, toMip);
}

//------------------------------------------------------------------------------
/**
*/
const VkImageView
RenderTextureGetVkImageView(const CoreGraphics::RenderTextureId id)
{
	return renderTextureAllocator.Get<1>(id.id24).view;
}

//------------------------------------------------------------------------------
/**
*/
const VkImage
RenderTextureGetVkImage(const CoreGraphics::RenderTextureId id)
{
	return renderTextureAllocator.Get<0>(id.id24).img;
}

//------------------------------------------------------------------------------
/**
	Internal helper function to generate mips, will assert that the texture is not within a pass
*/
void
RenderTextureGenerateMipHelper(const CoreGraphics::RenderTextureId id, IndexT from, const CoreGraphics::RenderTextureId target, IndexT to)
{
	const VkRenderTextureLoadInfo& loadInfoFrom = renderTextureAllocator.Get<0>(id.id24);
	const VkRenderTextureLoadInfo& loadInfoTo = renderTextureAllocator.Get<0>(target.id24);
	const VkRenderTextureRuntimeInfo& rtInfo = renderTextureAllocator.Get<1>(id.id24);

	n_assert(!rtInfo.inpass);
	n_assert(loadInfoFrom.format == loadInfoTo.format);

	// setup from-region
	int32_t fromMipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(loadInfoFrom.dims.width / Math::n_pow(2, (float)from)));
	int32_t fromMipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(loadInfoFrom.dims.height / Math::n_pow(2, (float)from)));
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
	VkUtilities::ImageLayoutTransition(GraphicsQueueType, VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::AllGraphicsShaders), VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::Transfer), VkUtilities::ImageMemoryBarrier(loadInfoFrom.img, fromSubres, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

	VkImageSubresourceRange toSubres;
	toSubres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	toSubres.baseArrayLayer = 0;
	toSubres.baseMipLevel = to;
	toSubres.layerCount = 1;
	toSubres.levelCount = 1;

	int32_t toMipWidth = (int32_t)Math::n_max(1.0f, Math::n_floor(loadInfoTo.dims.width / Math::n_pow(2, (float)to)));
	int32_t toMipHeight = (int32_t)Math::n_max(1.0f, Math::n_floor(loadInfoTo.dims.height / Math::n_pow(2, (float)to)));
	Math::rectangle<int> toRegion;
	toRegion.left = 0;
	toRegion.top = 0;
	toRegion.right = toMipWidth;
	toRegion.bottom = toMipHeight;
	
	// transition the texture to destination, blit, and transition it back
	VkUtilities::ImageLayoutTransition(GraphicsQueueType, VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::AllGraphicsShaders), VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::Transfer), VkUtilities::ImageMemoryBarrier(loadInfoTo.img, toSubres, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
	Vulkan::Blit(loadInfoFrom.img, fromRegion, from, loadInfoTo.img, toRegion, to);
	VkUtilities::ImageLayoutTransition(GraphicsQueueType, VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::Transfer), VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::AllGraphicsShaders), VkUtilities::ImageMemoryBarrier(loadInfoTo.img, toSubres, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	// transition source out from being blit source
	VkUtilities::ImageLayoutTransition(GraphicsQueueType, VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::Transfer), VkTypes::AsVkPipelineFlags(CoreGraphics::BarrierDependency::AllGraphicsShaders), VkUtilities::ImageMemoryBarrier(loadInfoFrom.img, fromSubres, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
}

} // namespace Vulkan