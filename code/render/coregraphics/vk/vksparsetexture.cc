//------------------------------------------------------------------------------
//  vksparsetexture.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vksparsetexture.h"
#include "vktypes.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
#include "vkshaderserver.h"
#include "coregraphics/graphicsdevice.h"
#include "vkcommandbuffer.h"
namespace Vulkan
{
VkSparseTextureAllocator vkSparseTextureAllocator;

//------------------------------------------------------------------------------
/**
*/
VkImage 
SparseTextureGetVkImage(const CoreGraphics::SparseTextureId id)
{
	return vkSparseTextureAllocator.Get<SparseTexture_Load>(id.id24).img;
}

//------------------------------------------------------------------------------
/**
*/
VkImageView 
SparseTextureGetVkImageView(const CoreGraphics::SparseTextureId id)
{
	return vkSparseTextureAllocator.Get<SparseTexture_Runtime>(id.id24).view;
}

} // namespace Vulkan

namespace CoreGraphics
{
using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
SparseTextureId 
CreateSparseTexture(const SparseTexureCreateInfo& info)
{
	Ids::Id32 id = vkSparseTextureAllocator.Alloc();

	using namespace Vulkan;

	TexturePageTable& table = vkSparseTextureAllocator.Get<SparseTexture_PageTable>(id);
	Util::Array<VkSparseMemoryBind>& opaqueBinds = vkSparseTextureAllocator.Get<SparseTexture_OpaqueBinds>(id);
	Util::Array<VkSparseImageMemoryBind>& pendingBinds = vkSparseTextureAllocator.Get<SparseTexture_PendingBinds>(id);
	Util::Array<CoreGraphics::Alloc>& allocs = vkSparseTextureAllocator.Get<SparseTexture_Allocs>(id);
	VkSparseTextureLoadInfo& loadInfo = vkSparseTextureAllocator.Get<SparseTexture_Load>(id);
	VkSparseTextureRuntimeInfo& runtimeInfo = vkSparseTextureAllocator.Get<SparseTexture_Runtime>(id);

	VkDevice dev = GetCurrentDevice();
	VkPhysicalDevice physDev = GetCurrentPhysicalDevice();
	VkFormat vkformat = VkTypes::AsVkFormat(info.format);
	VkImageType type = VkTypes::AsVkImageType(info.type);
	VkSampleCountFlagBits samples = VkTypes::AsVkSampleFlags(info.samples);

	// create image
	VkExtent3D extents;

	extents.width = info.width;
	extents.height = info.height;
	extents.depth = info.depth;
	VkImageCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT,
		type,
		vkformat,
		extents,
		info.mips,
		info.layers,
		samples,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0, nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkImage image;
	VkResult res = vkCreateImage(dev, &createInfo, nullptr, &image);
	n_assert(res == VK_SUCCESS);

	VkMemoryRequirements memoryReqs;
	vkGetImageMemoryRequirements(dev, image, &memoryReqs);

	VkPhysicalDeviceProperties devProps = GetCurrentProperties();
	n_assert(memoryReqs.size < devProps.limits.sparseAddressSpaceSize);

	// get sparse memory requirements
	uint32_t sparseMemoryRequirementsCount;
	VkSparseImageMemoryRequirements* sparseMemoryRequirements = nullptr;
	vkGetImageSparseMemoryRequirements(dev, image, &sparseMemoryRequirementsCount, nullptr);
	n_assert(sparseMemoryRequirementsCount > 0);
	sparseMemoryRequirements = n_new_array(VkSparseImageMemoryRequirements, sparseMemoryRequirementsCount);
	vkGetImageSparseMemoryRequirements(dev, image, &sparseMemoryRequirementsCount, sparseMemoryRequirements);

	uint32_t usedMemoryRequirements = UINT32_MAX;
	for (uint32_t i = 0; i < sparseMemoryRequirementsCount; i++)
	{
		if (sparseMemoryRequirements[i].formatProperties.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			usedMemoryRequirements = i;
			break;
		}
	}
	n_assert2(usedMemoryRequirements != UINT32_MAX, "No sparse image support for color textures");

	uint32_t memtype;
	res = GetMemoryType(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memtype);
	n_assert(res == VK_SUCCESS);

	uint32_t sparseBindsCount = memoryReqs.size / memoryReqs.alignment;
	Util::FixedArray<VkSparseMemoryBind> sparseMemoryBinds(sparseBindsCount);

	VkSparseImageMemoryRequirements sparseMemoryRequirement = sparseMemoryRequirements[usedMemoryRequirements];
	bool singleMipTail = sparseMemoryRequirement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;

	table.pages.Resize(info.layers);
	loadInfo.bindCounts.Resize(info.layers);
	for (IndexT i = 0; i < info.layers; i++)
	{
		table.pages[i].Resize(sparseMemoryRequirement.imageMipTailFirstLod);
		loadInfo.bindCounts[i].Resize(sparseMemoryRequirement.imageMipTailFirstLod);
	}

	// create sparse bindings, 
	for (SizeT layer = 0; layer < info.layers; layer++)
	{
		for (SizeT mip = 0; mip < (SizeT)sparseMemoryRequirement.imageMipTailFirstLod; mip++)
		{
			VkExtent3D extent;
			extent.width = Math::n_max(createInfo.extent.width >> mip, 1u);
			extent.height = Math::n_max(createInfo.extent.height >> mip, 1u);
			extent.depth = Math::n_max(createInfo.extent.depth >> mip, 1u);

			VkImageSubresource subres;
			subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subres.mipLevel = mip;
			subres.arrayLayer = layer;

			loadInfo.bindCounts[layer][mip].Resize(3);

			VkExtent3D granularity = sparseMemoryRequirement.formatProperties.imageGranularity;
			loadInfo.bindCounts[layer][mip][0] = extent.width / granularity.width + ((extent.width % granularity.width) ? 1u : 0u);
			loadInfo.bindCounts[layer][mip][1] = extent.height / granularity.height + ((extent.height % granularity.height) ? 1u : 0u);
			loadInfo.bindCounts[layer][mip][2] = extent.depth / granularity.depth + ((extent.depth % granularity.depth) ? 1u : 0u);
			uint32_t lastBlockExtent[3];
			lastBlockExtent[0] = (extent.width % granularity.width) ? extent.width % granularity.width : granularity.width;
			lastBlockExtent[1] = (extent.height % granularity.height) ? extent.height % granularity.height : granularity.height;
			lastBlockExtent[2] = (extent.depth % granularity.depth) ? extent.depth % granularity.depth : granularity.depth;

			// setup memory pages
			for (uint32_t z = 0; z < loadInfo.bindCounts[layer][mip][2]; z++)
			{
				for (uint32_t y = 0; y < loadInfo.bindCounts[layer][mip][1]; y++)
				{
					for (uint32_t x = 0; x < loadInfo.bindCounts[layer][mip][0]; x++)
					{
						VkOffset3D offset;
						offset.x = x * granularity.width;
						offset.y = y * granularity.height;
						offset.z = z * granularity.depth;

						VkExtent3D extent;
						extent.width = (x == loadInfo.bindCounts[layer][mip][0] - 1) ? lastBlockExtent[0] : granularity.width;
						extent.height = (y == loadInfo.bindCounts[layer][mip][1] - 1) ? lastBlockExtent[1] : granularity.height;
						extent.depth = (z == loadInfo.bindCounts[layer][mip][2] - 1) ? lastBlockExtent[2] : granularity.depth;

						VkSparseImageMemoryBind sparseBind;
						sparseBind.extent = extent;
						sparseBind.offset = offset;
						sparseBind.subresource = subres;
						sparseBind.memory = VK_NULL_HANDLE;
						sparseBind.memoryOffset = 0;
						sparseBind.flags = 0;

						// create new virtual page
						TexturePage page;
						page.size = memoryReqs.alignment;
						page.binding = sparseBind;
						page.offset = offset;
						page.extent = extent;
						page.mip = mip;
						page.layer = layer;
						page.alloc = CoreGraphics::Alloc{ VK_NULL_HANDLE, 0, 0, CoreGraphics::ImageMemory_Local };
						page.refCount = 0;
						pendingBinds.Append(sparseBind);
						table.pages[layer][mip].Append(page);

					}
				}
			}
		}

		// allocate memory if texture only has one mip tail per layer
		if ((!singleMipTail) && sparseMemoryRequirement.imageMipTailFirstLod < (uint32_t)info.mips)
		{
			CoreGraphics::Alloc alloc = CoreGraphics::AllocateMemory(dev, memoryReqs.alignment, sparseMemoryRequirement.imageMipTailSize);
			allocs.Append(alloc);

			VkSparseMemoryBind sparseBind;
			sparseBind.resourceOffset = sparseMemoryRequirement.imageMipTailOffset + layer * sparseMemoryRequirement.imageMipTailStride;
			sparseBind.size = sparseMemoryRequirement.imageMipTailSize;
			sparseBind.memory = alloc.mem;
			sparseBind.memoryOffset = alloc.offset;
			sparseBind.flags = 0;

			// add to opaque bindings
			opaqueBinds.Append(sparseBind);
		}
	}

	if ((singleMipTail) && sparseMemoryRequirement.imageMipTailFirstLod < (uint32_t)info.mips)
	{
		CoreGraphics::Alloc alloc = CoreGraphics::AllocateMemory(dev, memoryReqs.alignment, sparseMemoryRequirement.imageMipTailSize);
		allocs.Append(alloc);

		VkSparseMemoryBind sparseBind;
		sparseBind.resourceOffset = sparseMemoryRequirement.imageMipTailOffset;
		sparseBind.size = sparseMemoryRequirement.imageMipTailSize;
		sparseBind.memory = alloc.mem;
		sparseBind.memoryOffset = alloc.offset;
		sparseBind.flags = 0;

		// add memory bind to update queue
		opaqueBinds.Append(sparseBind);
	}

	VkImageSubresourceRange subres;
	subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subres.baseMipLevel = 0;
	subres.levelCount = info.mips;
	subres.baseArrayLayer = 0;
	subres.layerCount = info.layers;

	VkComponentMapping mapping;
	mapping.r = VK_COMPONENT_SWIZZLE_R;
	mapping.g = VK_COMPONENT_SWIZZLE_G;
	mapping.b = VK_COMPONENT_SWIZZLE_B;
	mapping.a = VK_COMPONENT_SWIZZLE_A;

	// create an image view
	VkImageViewCreateInfo viewCreateInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		image,
		VkTypes::AsVkImageViewType(info.type),
		vkformat,
		mapping,
		subres
	};
	VkImageView view;
	res = vkCreateImageView(dev, &viewCreateInfo, nullptr, &view);
	n_assert(res == VK_SUCCESS);

	loadInfo.dims = TextureDimensions{ info.width, info.height, info.depth };
	loadInfo.img = image;
	loadInfo.sparseMemoryRequirements = sparseMemoryRequirements[usedMemoryRequirements];
	runtimeInfo.view = view;
	n_delete_array(sparseMemoryRequirements);

	CoreGraphics::SparseTextureId ret;
	ret.id8 = SparseTextureIdType;
	ret.id24 = id;

	if (info.bindless)
		runtimeInfo.bind = VkShaderServer::Instance()->RegisterTexture(ret, info.type);
	else
		runtimeInfo.bind = 0;

	// make an initial commit for this texture
	CoreGraphics::SparseTextureCommitChanges(ret);

	
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
uint
SparseTextureGetBindlessHandle(const CoreGraphics::SparseTextureId id)
{
	return vkSparseTextureAllocator.Get<SparseTexture_Runtime>(id.id24).bind;
}

//------------------------------------------------------------------------------
/**
	@param id Is the sparse texture to update
	@param region Is a square in the sparse texture where the update should take place
	@param mip Is the mip level of that region to update
	@param tex Is the texture to load into the tiles from
	@param texMip Is the texture source mip to pick pixels from
*/
void 
SparseTextureMakeResident(const CoreGraphics::SparseTextureId id, const Math::rectangle<int>& region, IndexT mip, const CoreGraphics::TextureId tex, IndexT texMip)
{
	const TexturePageTable& table = vkSparseTextureAllocator.Get<SparseTexture_PageTable>(id.id24);
	VkSparseTextureLoadInfo& loadInfo = vkSparseTextureAllocator.Get<SparseTexture_Load>(id.id24);
	Util::Array<VkSparseImageMemoryBind>& pageBinds = vkSparseTextureAllocator.Get<SparseTexture_PendingBinds>(id.id24);

	VkDevice dev = GetCurrentDevice();

	TextureDimensions dims = TextureGetDimensions(tex);
	SizeT mipWidth = dims.width >> texMip;
	SizeT mipHeight = dims.height >> texMip;
	const uint32_t strideX = loadInfo.sparseMemoryRequirements.formatProperties.imageGranularity.width;
	const uint32_t strideY = loadInfo.sparseMemoryRequirements.formatProperties.imageGranularity.height;

	// calculate page ranges offset by mip
	uint32_t offsetX = region.left >> mip;
	uint32_t rangeX = Math::n_min(region.width() >> mip, (int)strideX);
	uint32_t endX = offsetX + (region.width() >> mip);

	uint32_t offsetY = region.top >> mip;
	uint32_t rangeY = Math::n_min(region.height() >> mip, (int)strideY);
	uint32_t endY = offsetY + (region.height() >> mip);

	Math::rectangle<int> fromRegion;
	fromRegion.left = 0;
	fromRegion.top = 0;
	fromRegion.right = mipWidth;
	fromRegion.bottom = mipHeight;

	VkImageSubresourceRange subresVirtual;
	subresVirtual.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresVirtual.baseArrayLayer = 0;
	subresVirtual.layerCount = 1;
	subresVirtual.baseMipLevel = mip;
	subresVirtual.levelCount = 1;

	VkImageSubresourceRange subresSource;
	subresSource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresSource.baseArrayLayer = 0;
	subresSource.layerCount = 1;
	subresSource.baseMipLevel = texMip;
	subresSource.levelCount = 1;

	// transition to transfer, use the handover context because it's a graphics context xD 
	CoreGraphics::LockResourceSubmission();
	CoreGraphics::SubmissionContextId sub = CoreGraphics::GetHandoverSubmissionContext();
	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		CoreGraphics::BarrierStage::Transfer,
		VkUtilities::ImageMemoryBarrier(loadInfo.img, subresVirtual, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));

	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		CoreGraphics::BarrierStage::Transfer,
		VkUtilities::ImageMemoryBarrier(TextureGetVkImage(tex), subresSource, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));

	uint32_t iteratorX = offsetX;
	uint32_t iteratorY = offsetY;
	while (iteratorY < endY)
	{
		iteratorX = offsetX;
		while (iteratorX < endX)
		{
			// calculate page index
			uint32_t pageIndex = iteratorX / strideX + iteratorY / strideY * loadInfo.bindCounts[0][mip][0];

			// get page and allocate memory
			TexturePage& page = table.pages[0][mip][pageIndex];

			// setup region
			Math::rectangle<int> toRegion;
			toRegion.left = iteratorX;
			toRegion.right = toRegion.left + rangeX;
			toRegion.top = iteratorY;
			toRegion.bottom = toRegion.top + rangeY;

			// if we already have memory, let it be
			if (page.alloc.mem == VK_NULL_HANDLE)
			{
				// allocate memory and append page update
				page.alloc = CoreGraphics::AllocateMemory(dev, page.size, page.size);
				page.binding.memory = page.alloc.mem;
				page.binding.memoryOffset = page.alloc.offset;
				page.refCount = 0;
				pageBinds.Append(page.binding);

				// if we just allocated the memory, write the whole range
				toRegion.left = page.offset.x;
				toRegion.right = page.offset.x + page.extent.width;
				toRegion.top = page.offset.y;
				toRegion.bottom = page.offset.y + page.extent.height;
			}
			page.refCount++;

			// update 

			VkImageBlit blit;
			blit.srcOffsets[0] = { fromRegion.left, fromRegion.top, 0 };
			blit.srcOffsets[1] = { fromRegion.right, fromRegion.bottom, 1 };
			blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)texMip, 0, 1 };
			blit.dstOffsets[0] = { toRegion.left, toRegion.top, 0 };
			blit.dstOffsets[1] = { toRegion.right, toRegion.bottom, 1 };
			blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)mip, 0, 1 };

			// perform blit
			vkCmdBlitImage(CommandBufferGetVk(CoreGraphics::SubmissionContextGetCmdBuffer(sub)),
				TextureGetVkImage(tex),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				loadInfo.img,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR);

			// jump a range
			iteratorX += rangeX;
		}

		// jump a range
		iteratorY += rangeY;
	}

	// transfer back to read from shaders
	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::Transfer,
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		VkUtilities::ImageMemoryBarrier(loadInfo.img, subresVirtual, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

	VkUtilities::ImageBarrier(CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::Transfer,
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		VkUtilities::ImageMemoryBarrier(TextureGetVkImage(tex), subresSource, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	CoreGraphics::UnlockResourceSubmission();
}

//------------------------------------------------------------------------------
/**
*/
void 
SparseTextureEvict(const CoreGraphics::SparseTextureId id, IndexT mip, const Math::rectangle<int>& region)
{
	const TexturePageTable& table = vkSparseTextureAllocator.Get<SparseTexture_PageTable>(id.id24);
	VkSparseTextureLoadInfo& loadInfo = vkSparseTextureAllocator.Get<SparseTexture_Load>(id.id24);
	Util::Array<VkSparseImageMemoryBind>& pageBinds = vkSparseTextureAllocator.Get<SparseTexture_PendingBinds>(id.id24);

	// calculate page ranges
	const uint32_t strideX = loadInfo.sparseMemoryRequirements.formatProperties.imageGranularity.width;
	const uint32_t strideY = loadInfo.sparseMemoryRequirements.formatProperties.imageGranularity.height;

	// calculate page ranges offset by mip
	uint32_t offsetX = region.left >> mip;
	uint32_t rangeX = Math::n_min(region.width() >> mip, (int)strideX);
	uint32_t endX = offsetX + (region.width() >> mip);

	uint32_t offsetY = region.top >> mip;
	uint32_t rangeY = Math::n_min(region.height() >> mip, (int)strideY);
	uint32_t endY = offsetY + (region.height() >> mip);

	uint32_t iteratorX = offsetX;
	uint32_t iteratorY = offsetY;
	while (iteratorY < endY)
	{
		iteratorX = offsetX;
		while (iteratorX < endX)
		{
			// calculate page index
			uint32_t pageIndex = iteratorX / strideX + iteratorY / strideY * loadInfo.bindCounts[0][mip][0];

			// get page and allocate memory
			TexturePage& page = table.pages[0][mip][pageIndex];

			// if we have an alloc, dealloc memory
			if (page.alloc.mem != VK_NULL_HANDLE)
			{
				page.refCount--;
				if (page.refCount == 0)
				{
					CoreGraphics::FreeMemory(page.alloc);
					page.alloc.mem = VK_NULL_HANDLE;
					page.alloc.offset = 0;
					page.binding.memory = VK_NULL_HANDLE;
					page.binding.memoryOffset = 0;

					// append pending page update
					pageBinds.Append(page.binding);
				}
			}

			// jump a range
			iteratorX += rangeX;
		}

		// jump a range
		iteratorY += rangeY;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SparseTextureCommitChanges(const CoreGraphics::SparseTextureId id)
{
	const VkSparseTextureLoadInfo& loadInfo = vkSparseTextureAllocator.Get<SparseTexture_Load>(id.id24);
	Util::Array<VkSparseMemoryBind>& opaqueBinds = vkSparseTextureAllocator.Get<SparseTexture_OpaqueBinds>(id.id24);
	Util::Array<VkSparseImageMemoryBind>& pageBinds = vkSparseTextureAllocator.Get<SparseTexture_PendingBinds>(id.id24);

	// abort early if we have no updates
	if (opaqueBinds.IsEmpty() && pageBinds.IsEmpty())
		return;

	// setup bind structs
	VkSparseImageMemoryBindInfo imageMemoryBindInfo =
	{
		loadInfo.img,
		pageBinds.Size(),
		pageBinds.Size() > 0 ? pageBinds.Begin() : nullptr
	};
	VkSparseImageOpaqueMemoryBindInfo opaqueMemoryBindInfo =
	{
		loadInfo.img,
		opaqueBinds.Size(),
		opaqueBinds.Size() > 0 ? opaqueBinds.Begin() : nullptr
	};
	VkBindSparseInfo bindInfo =
	{
		VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,
		nullptr,
		0, nullptr,
		0, nullptr,
		opaqueBinds.IsEmpty() ? 0 : 1, &opaqueMemoryBindInfo,
		pageBinds.IsEmpty() ? 0 : 1, &imageMemoryBindInfo,
		0, nullptr
	};

	// execute sparse bind, the 
	Vulkan::SparseTextureBind(bindInfo);

	// clear all pending binds
	pageBinds.Clear();
	opaqueBinds.Clear();
}

} // namespace CoreGraphics
