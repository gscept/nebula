//------------------------------------------------------------------------------
// vkmemorytextureloader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkmemorytexturepool.h"
#include "coregraphics/texture.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "vkutilities.h"
#include "resources/resourcemanager.h"
#include "vkshaderserver.h"
#include "vkcommandbuffer.h"
#include "coregraphics/submissioncontext.h"
#include "vksubmissioncontext.h"
#include "coregraphics/glfw/glfwwindow.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryTexturePool, 'VKTO', Resources::ResourceMemoryPool);

//------------------------------------------------------------------------------
/**
*/
ResourcePool::LoadStatus
VkMemoryTexturePool::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
	const TextureCreateInfo* data = (const TextureCreateInfo*)info;

	/// during the load-phase, we can safetly get the structs
	this->EnterGet();
	VkTextureRuntimeInfo& runtimeInfo = this->Get<Texture_RuntimeInfo>(id.resourceId);
	VkTextureLoadInfo& loadInfo = this->Get<Texture_LoadInfo>(id.resourceId);
	VkTextureWindowInfo& windowInfo = this->Get<Texture_WindowInfo>(id.resourceId);

	// create adjusted info
	TextureCreateInfoAdjusted adjustedInfo = TextureGetAdjustedInfo(*data);

	VkFormat vkformat;

	if (adjustedInfo.usage & TextureUsage::RenderUsage)
		vkformat = VkTypes::AsVkFramebufferFormat(data->format);
	else if (adjustedInfo.usage & TextureUsage::ReadWriteUsage)
		vkformat = VkTypes::AsVkDataFormat(data->format);
	else
		vkformat = VkTypes::AsVkFormat(data->format);


	VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
	VkDevice dev = Vulkan::GetCurrentDevice();
	loadInfo.dev = dev;

	VkFormatProperties formatProps;
	vkGetPhysicalDeviceFormatProperties(physicalDev, vkformat, &formatProps);
	VkExtent3D extents;
	extents.width = adjustedInfo.width;
	extents.height = adjustedInfo.height;
	extents.depth = adjustedInfo.depth;

	bool isDepthFormat = VkTypes::IsDepthFormat(adjustedInfo.format);

	// setup usage flags, by default, all textures can be sampled from
	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	if (adjustedInfo.usage & TextureUsage::ImmutableUsage)
	{
		n_assert_fmt(adjustedInfo.usage == TextureUsage::ImmutableUsage, "Texture with immutable usage may not use any other flags");
		usage |=  VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (adjustedInfo.usage & TextureUsage::RenderUsage)
	{
		n_assert_fmt((adjustedInfo.usage & TextureUsage::ReadWriteUsage) == 0, "Texture may not be used for render and readwrite at the same time, create alias to support it");
		usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | (isDepthFormat ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (adjustedInfo.usage & TextureUsage::ReadWriteUsage)
	{ 
		n_assert_fmt((adjustedInfo.usage & TextureUsage::RenderUsage) == 0, "Texture may not be used for render and readwrite at the same time, create alias to support it");
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	if (adjustedInfo.usage & TextureUsage::CopyUsage)
		usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (!adjustedInfo.windowTexture)
	{
		VkSampleCountFlagBits samples;
		switch (adjustedInfo.samples)
		{
		case 1:
			samples = VK_SAMPLE_COUNT_1_BIT;
			break;
		case 2:
			samples = VK_SAMPLE_COUNT_2_BIT;
			break;
		case 4:
			samples = VK_SAMPLE_COUNT_4_BIT;
			break;
		case 8:
			samples = VK_SAMPLE_COUNT_8_BIT;
			break;
		case 16:
			samples = VK_SAMPLE_COUNT_16_BIT;
			break;
		case 32:
			samples = VK_SAMPLE_COUNT_32_BIT;
			break;
		case 64:
			samples = VK_SAMPLE_COUNT_64_BIT;
			break;
		default:
			n_error("Invalid number of samples '%d'", adjustedInfo.samples);
			break;
		}

		VkImageViewType viewType;
		VkImageType type;
		switch (adjustedInfo.type)
		{
		case Texture1D:
			type = VK_IMAGE_TYPE_1D;
			viewType = VK_IMAGE_VIEW_TYPE_1D;
			break;
		case Texture1DArray:
			type = VK_IMAGE_TYPE_1D;
			viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			break;
		case Texture2D:
			type = VK_IMAGE_TYPE_2D;
			viewType = VK_IMAGE_VIEW_TYPE_2D;
			break;
		case Texture2DArray:
			type = VK_IMAGE_TYPE_2D;
			viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			break;
		case TextureCube:
			type = VK_IMAGE_TYPE_2D;
			viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			break;
		case TextureCubeArray:
			type = VK_IMAGE_TYPE_2D;
			viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			break;
		case Texture3D:
			type = VK_IMAGE_TYPE_3D;
			viewType = VK_IMAGE_VIEW_TYPE_3D;
			break;
		}

		// if read-write, we will almost definitely use this texture on multiple queues
		VkSharingMode sharingMode = (adjustedInfo.usage & TextureUsage::ReadWriteUsage) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
		const Util::Set<uint32_t>& queues = Vulkan::GetQueueFamilies();

		VkImageCreateFlags createFlags = 0;

		if (adjustedInfo.alias != CoreGraphics::TextureId::Invalid())
			createFlags |= VK_IMAGE_CREATE_ALIAS_BIT;
		if (viewType == VK_IMAGE_VIEW_TYPE_CUBE || viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
			createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		VkImageCreateInfo imgInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			createFlags,
			type,
			vkformat,
			extents,
			adjustedInfo.mips,
			adjustedInfo.layers,
			samples,
			VK_IMAGE_TILING_OPTIMAL,
			usage,
			sharingMode,
			sharingMode == VK_SHARING_MODE_CONCURRENT ? queues.Size() : 0,
			sharingMode == VK_SHARING_MODE_CONCURRENT ? queues.KeysAsArray().Begin() : nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
		};

		VkResult stat = vkCreateImage(dev, &imgInfo, nullptr, &loadInfo.img);
		n_assert(stat == VK_SUCCESS);

		// if we don't use aliasing, create new memory
		if (adjustedInfo.alias == CoreGraphics::TextureId::Invalid())
		{
			// allocate memory backing
			uint32_t alignedSize;
			VkUtilities::AllocateImageMemory(loadInfo.dev, loadInfo.img, loadInfo.mem, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, alignedSize);
			vkBindImageMemory(dev, loadInfo.img, loadInfo.mem, 0);
		}
		else
		{
			// otherwise use other image memory to create alias
			VkDeviceMemory mem = this->Get<Texture_LoadInfo>(adjustedInfo.alias.resourceId).mem;
			loadInfo.mem = mem;
			vkBindImageMemory(dev, loadInfo.img, loadInfo.mem, 0);
		}

		// if we have initial data to setup, perform a data transfer
		if (adjustedInfo.buffer != nullptr)
		{
			// use resource submission
			CoreGraphics::SubmissionContextId sub = CoreGraphics::GetResourceSubmissionContext();

			// transition into transfer mode
			VkImageSubresourceRange subres;
			subres.aspectMask = isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			subres.baseArrayLayer = 0;
			subres.baseMipLevel = 0;
			subres.layerCount = adjustedInfo.layers;
			subres.levelCount = adjustedInfo.mips;


			// insert barrier
			VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
				CoreGraphics::BarrierStage::Host,
				CoreGraphics::BarrierStage::Transfer,
				VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

			// add image update, take the output buffer and memory and add to delayed delete
			VkBuffer outBuf;
			VkDeviceMemory outMem;
			uint32_t size = PixelFormat::ToSize(data->format);
			VkUtilities::ImageUpdate(dev, CoreGraphics::SubmissionContextGetCmdBuffer(sub), TransferQueueType, loadInfo.img, imgInfo, 0, 0, VkDeviceSize(adjustedInfo.width * adjustedInfo.height * adjustedInfo.depth * size), (uint32_t*)adjustedInfo.buffer, outBuf, outMem);

			// add host memory buffer, intermediate device memory, and intermediate device buffer to delete queue
			SubmissionContextFreeDeviceMemory(sub, dev, outMem);
			SubmissionContextFreeBuffer(sub, dev, outBuf);

			// transition image to be used for rendering
			VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
				CoreGraphics::BarrierStage::Transfer,
				CoreGraphics::BarrierStage::AllGraphicsShaders,
				VkUtilities::ImageMemoryBarrier(loadInfo.img, subres, TransferQueueType, GraphicsQueueType, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}

		// if used for render, find appropriate renderable format
		if (adjustedInfo.usage & TextureUsage::RenderUsage)
		{
			vkformat = VkTypes::AsVkFramebufferFormat(adjustedInfo.format);
			if (!isDepthFormat)
			{
				VkFormatProperties formatProps;
				vkGetPhysicalDeviceFormatProperties(Vulkan::GetCurrentPhysicalDevice(), vkformat, &formatProps);
				n_assert(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT &&
					formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT &&
					formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
			}
			else
				loadInfo.defaultLayout = CoreGraphicsImageLayout::DepthStencilRead;
		}

		// create view
		VkImageSubresourceRange viewRange;
		viewRange.aspectMask = isDepthFormat ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT; // view only supports reading depth in shader
		viewRange.baseMipLevel = 0;
		viewRange.levelCount = adjustedInfo.mips;
		viewRange.baseArrayLayer = 0;
		viewRange.layerCount = adjustedInfo.layers;
		VkImageViewCreateInfo viewCreate =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			loadInfo.img,
			viewType,
			vkformat,
			VkTypes::AsVkMapping(adjustedInfo.format),
			viewRange
		};
		stat = vkCreateImageView(dev, &viewCreate, nullptr, &runtimeInfo.view);
		n_assert(stat == VK_SUCCESS);

		// use setup submission
		CoreGraphics::SubmissionContextId sub = CoreGraphics::GetSetupSubmissionContext();
		
		if (adjustedInfo.usage & TextureUsage::RenderUsage)
		{
			// perform initial clear if render target
			if (!isDepthFormat)
			{
				VkClearColorValue clear = { 0, 0, 0, 0 };
				VkUtilities::ImageBarrier(SubmissionContextGetCmdBuffer(sub), CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
				VkUtilities::ImageColorClear(SubmissionContextGetCmdBuffer(sub), loadInfo.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, viewRange);
				VkUtilities::ImageBarrier(SubmissionContextGetCmdBuffer(sub), CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::PassOutput, VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			}
			else
			{
				// clear image and transition layout
				VkClearDepthStencilValue clear = { 1, 0 };
				VkImageSubresourceRange clearRange = viewRange;
				clearRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				VkUtilities::ImageBarrier(SubmissionContextGetCmdBuffer(sub), CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(loadInfo.img, clearRange, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
				VkUtilities::ImageDepthStencilClear(SubmissionContextGetCmdBuffer(sub), loadInfo.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, clearRange);
				VkUtilities::ImageBarrier(SubmissionContextGetCmdBuffer(sub), CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::PassOutput, VkUtilities::ImageMemoryBarrier(loadInfo.img, clearRange, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
			}
		}
		else if (adjustedInfo.usage & TextureUsage::ReadWriteUsage)
		{
			// insert barrier to transition into a useable state
			VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
				CoreGraphics::BarrierStage::Host,
				CoreGraphics::BarrierStage::AllGraphicsShaders,
				VkUtilities::ImageMemoryBarrier(loadInfo.img, viewRange, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		}

		// register image with shader server
		if (adjustedInfo.bindless)
			runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(TextureId(id), isDepthFormat, adjustedInfo.type);
		else
			runtimeInfo.bind = 0;
		runtimeInfo.type = adjustedInfo.type;

	}
	else // setup as window texture
	{
		// get submission context
		n_assert(adjustedInfo.window != CoreGraphics::WindowId::Invalid());
		CoreGraphics::SubmissionContextId sub = CoreGraphics::GetSetupSubmissionContext();

		VkBackbufferInfo& backbufferInfo = CoreGraphics::glfwWindowAllocator.Get<GLFW_Backbuffer>(adjustedInfo.window.id24);
		windowInfo.swapimages = backbufferInfo.backbuffers;
		windowInfo.swapviews = backbufferInfo.backbufferViews;
		windowInfo.window = adjustedInfo.window;
		VkClearColorValue clear = { 0, 0, 0, 0 };

		VkImageSubresourceRange subres;
		subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subres.baseArrayLayer = 0;
		subres.baseMipLevel = 0;
		subres.layerCount = 1;
		subres.levelCount = 1;

		// clear textures
		IndexT i;
		for (i = 0; i < windowInfo.swapimages.Size(); i++)
		{
			VkUtilities::ImageBarrier(SubmissionContextGetCmdBuffer(sub), CoreGraphics::BarrierStage::Host, CoreGraphics::BarrierStage::Transfer, VkUtilities::ImageMemoryBarrier(windowInfo.swapimages[i], subres, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
			VkUtilities::ImageColorClear(SubmissionContextGetCmdBuffer(sub), windowInfo.swapimages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clear, subres);
			VkUtilities::ImageBarrier(SubmissionContextGetCmdBuffer(sub), CoreGraphics::BarrierStage::Transfer, CoreGraphics::BarrierStage::PassOutput, VkUtilities::ImageMemoryBarrier(windowInfo.swapimages[i], subres, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
		}

		n_assert(adjustedInfo.type == Texture2D);
		n_assert(adjustedInfo.mips == 1);
		n_assert(adjustedInfo.samples == 1);

		loadInfo.img = windowInfo.swapimages[0];
		loadInfo.mem = VK_NULL_HANDLE;
		
		runtimeInfo.type = Texture2D;
		runtimeInfo.view = backbufferInfo.backbufferViews[0];
	}

	loadInfo.dims.width = adjustedInfo.width;
	loadInfo.dims.height = adjustedInfo.height;
	loadInfo.dims.depth = adjustedInfo.depth;
	loadInfo.mips = adjustedInfo.mips;
	loadInfo.layers = adjustedInfo.layers;
	loadInfo.format = adjustedInfo.format;
	loadInfo.dev = dev;
	loadInfo.texUsage = adjustedInfo.usage;
	loadInfo.alias = adjustedInfo.alias;
	loadInfo.samples = adjustedInfo.samples;
	loadInfo.defaultLayout = adjustedInfo.defaultLayout;

	this->LeaveGet();
	
	CoreGraphics::RegisterTexture(adjustedInfo.name, id);

	n_assert(this->GetState(id) == Resource::Pending);
	n_assert(loadInfo.img != VK_NULL_HANDLE);

	// set loaded flag
	this->states[id.poolId] = Resources::Resource::Loaded;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName((TextureId)id, adjustedInfo.name.Value());
#endif

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::GenerateMipmaps(const CoreGraphics::TextureId id)
{
	n_error("IMPLEMENT ME!");
}

//------------------------------------------------------------------------------
/**
*/
bool
VkMemoryTexturePool::Map(const CoreGraphics::TextureId id, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo & outMapInfo)
{
	textureAllocator.EnterGet();
	VkTextureRuntimeInfo& runtime = textureAllocator.Get<0>(id.resourceId);
	VkTextureLoadInfo& load = textureAllocator.Get<1>(id.resourceId);
	VkTextureMappingInfo& map = textureAllocator.Get<2>(id.resourceId);

	bool retval = false;
	if (Texture2D == runtime.type)
	{
		VkFormat vkformat = VkTypes::AsVkFormat(load.format);
		VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
		uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

		uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.width / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.height / Math::n_pow(2, (float)mipLevel)));

		map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		map.region.dstOffset = { 0, 0, 0 };
		map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 0, 1 };
		map.region.srcOffset = { 0, 0, 0 };
		map.region.extent = { mipWidth, mipHeight, 1 };
		uint32_t memSize;
		VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, memSize, map.mem, map.buf);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.mipWidth = mipWidth;
		outMapInfo.mipHeight = mipHeight;
		outMapInfo.rowPitch = (int32_t)memSize / mipHeight;
		outMapInfo.depthPitch = (int32_t)memSize;
		VkResult res = vkMapMemory(load.dev, map.mem, 0, (int32_t)memSize, 0, &outMapInfo.data);
		n_assert(res == VK_SUCCESS);
		retval = res == VK_SUCCESS;
		map.mapCount++;
	}
	else if (Texture3D == runtime.type)
	{
		VkFormat vkformat = VkTypes::AsVkFormat(load.format);
		VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
		uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

		uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.width / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.height / Math::n_pow(2, (float)mipLevel)));
		uint32_t mipDepth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.depth / Math::n_pow(2, (float)mipLevel)));

		map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		map.region.dstOffset = { 0, 0, 0 };
		map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, 1, 1 };
		map.region.srcOffset = { 0, 0, 0 };
		map.region.extent = { mipWidth, mipHeight, mipDepth };
		uint32_t memSize;
		VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, memSize, map.mem, map.buf);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.mipWidth = mipWidth;
		outMapInfo.mipHeight = mipHeight;
		outMapInfo.rowPitch = (int32_t)memSize / mipWidth;
		outMapInfo.depthPitch = (int32_t)memSize;
		VkResult res = vkMapMemory(load.dev, map.mem, 0, (int32_t)memSize, 0, &outMapInfo.data);
		n_assert(res == VK_SUCCESS);
		retval = res == VK_SUCCESS;
		map.mapCount++;
	}
	textureAllocator.LeaveGet();
	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::Unmap(const CoreGraphics::TextureId id, IndexT mipLevel)
{
	textureAllocator.EnterGet();
	VkTextureRuntimeInfo& runtime = textureAllocator.Get<0>(id.resourceId);
	VkTextureLoadInfo& load = textureAllocator.Get<1>(id.resourceId);
	VkTextureMappingInfo& map = textureAllocator.Get<2>(id.resourceId);

	// unmap and dealloc
	vkUnmapMemory(load.dev, load.mem);
	VkUtilities::WriteImage(map.buf, load.img, map.region);
	map.mapCount--;
	if (map.mapCount == 0)
	{
		vkFreeMemory(load.dev, map.mem, nullptr);
		vkDestroyBuffer(load.dev, map.buf, nullptr);
	}

	textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
bool
VkMemoryTexturePool::MapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel, CoreGraphics::GpuBufferTypes::MapType mapType, CoreGraphics::TextureMapInfo & outMapInfo)
{
	textureAllocator.EnterGet();
	VkTextureRuntimeInfo& runtime = textureAllocator.Get<0>(id.resourceId);
	VkTextureLoadInfo& load = textureAllocator.Get<1>(id.resourceId);
	VkTextureMappingInfo& map = textureAllocator.Get<2>(id.resourceId);

	bool retval = false;

	VkFormat vkformat = VkTypes::AsVkFormat(load.format);
	VkTypes::VkBlockDimensions blockSize = VkTypes::AsVkBlockSize(vkformat);
	uint32_t size = CoreGraphics::PixelFormat::ToSize(load.format);

	uint32_t mipWidth = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.width / Math::n_pow(2, (float)mipLevel)));
	uint32_t mipHeight = (uint32_t)Math::n_max(1.0f, Math::n_ceil(load.dims.height / Math::n_pow(2, (float)mipLevel)));

	map.region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	map.region.dstOffset = { 0, 0, 0 };
	map.region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mipLevel, (uint32_t)face, 1 };
	map.region.srcOffset = { 0, 0, 0 };
	map.region.extent = { mipWidth, mipHeight, 1 };
	uint32_t memSize;
	VkUtilities::ReadImage(load.img, load.format, load.dims, runtime.type, map.region, memSize, map.mem, map.buf);

	// the row pitch must be the size of one pixel times the number of pixels in width
	outMapInfo.mipWidth = mipWidth;
	outMapInfo.mipHeight = mipHeight;
	outMapInfo.rowPitch = (int32_t)memSize / mipWidth;
	outMapInfo.depthPitch = (int32_t)memSize;
	VkResult res = vkMapMemory(load.dev, map.mem, 0, (int32_t)memSize, 0, &outMapInfo.data);
	n_assert(res == VK_SUCCESS);
	retval = res == VK_SUCCESS;
	map.mapCount++;

	textureAllocator.LeaveGet();

	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::UnmapCubeFace(const CoreGraphics::TextureId id, CoreGraphics::TextureCubeFace face, IndexT mipLevel)
{
	textureAllocator.EnterGet();
	VkTextureRuntimeInfo& runtime = textureAllocator.Get<0>(id.resourceId);
	VkTextureLoadInfo& load = textureAllocator.Get<1>(id.resourceId);
	VkTextureMappingInfo& map = textureAllocator.Get<2>(id.resourceId);

	// unmap and dealloc
	vkUnmapMemory(load.dev, load.mem);
	VkUtilities::WriteImage(map.buf, load.img, map.region);
	map.mapCount--;
	if (map.mapCount == 0)
	{
		vkFreeMemory(load.dev, map.mem, nullptr);
		vkDestroyBuffer(load.dev, map.buf, nullptr);
	}

	textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::Update(const CoreGraphics::TextureId id, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = width;
	copy.imageExtent.height = height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = left;
	copy.imageOffset.y = top;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / width;
	copy.bufferImageHeight = height;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::Update(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void* data, SizeT dataSize, IndexT mip)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = dims.width;
	copy.imageExtent.height = dims.height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = 0;
	copy.imageOffset.y = 0;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = 0;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / dims.width;
	copy.bufferImageHeight = dims.height;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::UpdateArray(const CoreGraphics::TextureId id, void* data, SizeT dataSize, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip, IndexT layer)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = width;
	copy.imageExtent.height = height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = left;
	copy.imageOffset.y = top;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = layer;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / width;
	copy.bufferImageHeight = height;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::UpdateArray(const CoreGraphics::TextureId id, CoreGraphics::TextureDimensions dims, void* data, SizeT dataSize, IndexT mip, IndexT layer)
{
	VkBufferImageCopy copy;
	copy.imageExtent.width = dims.width;
	copy.imageExtent.height = dims.height;
	copy.imageExtent.depth = 1;			// hmm, might want this for cube maps and volume textures too
	copy.imageOffset.x = 0;
	copy.imageOffset.y = 0;
	copy.imageOffset.z = 0;
	copy.imageSubresource.mipLevel = mip;
	copy.imageSubresource.layerCount = 1;
	copy.imageSubresource.baseArrayLayer = layer;
	copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.bufferOffset = 0;
	copy.bufferRowLength = dataSize / dims.width;
	copy.bufferImageHeight = dims.height;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryTexturePool::Copy(const CoreGraphics::TextureId from, const CoreGraphics::TextureId to, SizeT width, SizeT height, SizeT depth, IndexT srcMip, IndexT srcLayer, SizeT srcXOffset, SizeT srcYOffset, SizeT srcZOffset, IndexT dstMip, IndexT dstLayer, SizeT dstXOffset, SizeT dstYOffset, SizeT dstZOffset)
{
	VkImageCopy copy;
	copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.srcSubresource.baseArrayLayer = srcLayer;
	copy.srcSubresource.layerCount = 1;
	copy.srcSubresource.mipLevel = srcMip;
	copy.srcOffset = { srcXOffset, srcYOffset, srcZOffset };

	copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy.dstSubresource.baseArrayLayer = dstLayer;
	copy.dstSubresource.layerCount = 1;
	copy.dstSubresource.mipLevel = dstMip;
	copy.dstOffset = { dstXOffset, dstYOffset, dstZOffset };

	copy.extent = { (uint32_t)width, (uint32_t)height, (uint32_t)depth };

	VkTextureLoadInfo& fromLoad = textureAllocator.GetSafe<1>(from.resourceId);
	VkTextureLoadInfo& toLoad = textureAllocator.GetSafe<1>(to.resourceId);

	// begin immediate action, this might actually be delayed but we can't really know from here
	CoreGraphics::CommandBufferId cmdBuf = VkUtilities::BeginImmediateTransfer();
	vkCmdCopyImage(CommandBufferGetVk(cmdBuf), fromLoad.img, VK_IMAGE_LAYOUT_GENERAL, toLoad.img, VK_IMAGE_LAYOUT_GENERAL, 1, &copy);
	VkUtilities::EndImmediateTransfer(cmdBuf);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureDimensions
VkMemoryTexturePool::GetDimensions(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).dims;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PixelFormat::Code
VkMemoryTexturePool::GetPixelFormat(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).format;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureType
VkMemoryTexturePool::GetType(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_RuntimeInfo>(id.resourceId).type;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureId 
VkMemoryTexturePool::GetAlias(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).alias;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureUsage 
VkMemoryTexturePool::GetUsageBits(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).texUsage;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
VkMemoryTexturePool::GetNumMips(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).mips;
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
VkMemoryTexturePool::GetNumLayers(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).layers;
}

//------------------------------------------------------------------------------
/**
*/
SizeT 
VkMemoryTexturePool::GetNumSamples(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).samples;
}

//------------------------------------------------------------------------------
/**
*/
uint 
VkMemoryTexturePool::GetBindlessHandle(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_RuntimeInfo>(id.resourceId).bind;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphicsImageLayout 
VkMemoryTexturePool::GetDefaultLayout(const CoreGraphics::TextureId id)
{
	return textureAllocator.GetSafe<Texture_LoadInfo>(id.resourceId).defaultLayout;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
VkMemoryTexturePool::SwapBuffers(const CoreGraphics::TextureId id)
{
	VkTextureLoadInfo& loadInfo = this->GetSafe<Texture_LoadInfo>(id.resourceId);
	VkTextureRuntimeInfo& runtimeInfo = this->GetSafe<Texture_RuntimeInfo>(id.resourceId);
	VkTextureWindowInfo& wnd = this->GetSafe<Texture_WindowInfo>(id.resourceId);
	n_assert(wnd.window != CoreGraphics::WindowId::Invalid());
	VkWindowSwapInfo& swapInfo = CoreGraphics::glfwWindowAllocator.Get<5>(wnd.window.id24);
	VkSemaphore sem = Vulkan::GetPresentSemaphore();
	VkResult res = vkAcquireNextImageKHR(Vulkan::GetCurrentDevice(), swapInfo.swapchain, UINT64_MAX, sem, VK_NULL_HANDLE, &swapInfo.currentBackbuffer);
	Vulkan::WaitForPresent(sem);
	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// this means our swapchain needs a resize!
	}
	else
	{
		n_assert(res == VK_SUCCESS);
	}

	// set image and update texture
	loadInfo.img = wnd.swapimages[swapInfo.currentBackbuffer];
	runtimeInfo.view = wnd.swapviews[swapInfo.currentBackbuffer];
	return swapInfo.currentBackbuffer;
}

} // namespace Vulkan