//------------------------------------------------------------------------------
//  vksparsetexture.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vksparsetexture.h"
#include "vktypes.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
namespace Vulkan
{
VkSparseTextureAllocator vkSparseTextureAllocator;
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

	VkBindSparseInfo& bindInfo = vkSparseTextureAllocator.Get<SparseTexture_BindInfos>(id);
	Util::Array<VkSparseMemoryBind>& opaqueBinds = vkSparseTextureAllocator.Get<SparseTexture_OpaqueBinds>(id);
	Util::Array<TexturePage>& pageBinds = vkSparseTextureAllocator.Get<SparseTexture_PageBinds>(id);

	VkDevice dev = GetCurrentDevice();
	VkPhysicalDevice physDev = GetCurrentPhysicalDevice();
	VkFormat vkformat = VkTypes::AsVkFormat(info.format);
	VkImageType type = VkTypes::AsVkImageType(info.type);
	VkSampleCountFlagBits samples = VkTypes::AsVkSampleFlags(info.samples);

	VkSparseImageFormatProperties* sparseProperties;
	uint32_t sparsePropertiesCount;
	vkGetPhysicalDeviceSparseImageFormatProperties(
		physDev,
		vkformat,
		type,
		samples,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		&sparsePropertiesCount,
		nullptr);
	n_assert(sparsePropertiesCount > 0);
	sparseProperties = n_new_array(VkSparseImageFormatProperties, sparsePropertiesCount);
	vkGetPhysicalDeviceSparseImageFormatProperties(
		GetCurrentPhysicalDevice(),
		vkformat,
		type,
		samples,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		&sparsePropertiesCount,
		sparseProperties);

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
	VkSparseImageMemoryRequirements* sparseMemoryRequirements;
	vkGetImageSparseMemoryRequirements(dev, image, &sparseMemoryRequirementsCount, nullptr);
	n_assert(sparseMemoryRequirementsCount > 0);
	sparseMemoryRequirements = n_new_array(VkSparseImageMemoryRequirements, sparseMemoryRequirementsCount);
	vkGetImageSparseMemoryRequirements(dev, image, &sparseMemoryRequirementsCount, sparseMemoryRequirements);

	uint32_t memtype;
	res = GetMemoryType(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memtype);
	n_assert(res == VK_SUCCESS);

	uint32_t sparseBindsCount = memoryReqs.size / memoryReqs.alignment;
	Util::FixedArray<VkSparseMemoryBind> sparseMemoryBinds(sparseBindsCount);

	VkSparseImageMemoryRequirements sparseMemoryRequirement = sparseMemoryRequirements[0];
	bool singleMipTail = sparseMemoryRequirement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;

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

			VkExtent3D granularity = sparseMemoryRequirement.formatProperties.imageGranularity;
			uint32_t sparseBindCounts[3];
			sparseBindCounts[0] = extent.width / granularity.width + ((extent.width & granularity.width) ? 1u : 0u);
			sparseBindCounts[1] = extent.height / granularity.height + ((extent.height & granularity.height) ? 1u : 0u);
			sparseBindCounts[2] = extent.depth / granularity.depth + ((extent.depth & granularity.depth) ? 1u : 0u);
			uint32_t lastBlockExtent[3];
			lastBlockExtent[0] = (extent.width % granularity.width) ? extent.width % granularity.width : granularity.width;
			lastBlockExtent[1] = (extent.height % granularity.height) ? extent.height % granularity.height : granularity.height;
			lastBlockExtent[2] = (extent.depth % granularity.depth) ? extent.depth % granularity.depth : granularity.depth;

			// setup memory pages
			for (uint32_t z = 0; z < sparseBindCounts[2]; z++)
			{
				for (uint32_t y = 0; y < sparseBindCounts[1]; y++)
				{
					for (uint32_t x = 0; x < sparseBindCounts[0]; x++)
					{
						VkOffset3D offset;
						offset.x = x * granularity.width;
						offset.y = y * granularity.height;
						offset.z = z * granularity.depth;

						VkExtent3D extent;
						extent.width = (x == sparseBindCounts[0] - 1) ? lastBlockExtent[0] : granularity.width;
						extent.height = (x == sparseBindCounts[1] - 1) ? lastBlockExtent[1] : granularity.height;
						extent.depth = (x == sparseBindCounts[2] - 1) ? lastBlockExtent[2] : granularity.depth;

						// create new virtual page
						/*
							VirtualPage page{ offset, extent, alignment, mip, layer, subres }
						*/
						VkSparseImageMemoryBind sparseBind;
						sparseBind.extent = extent;
						sparseBind.offset = offset;
						sparseBind.subresource = subres;

						TexturePage page;
						page.alignment = memoryReqs.alignment;
						page.binding = sparseBind;
						pageBinds.Append(page);
					}
				}
			}
		}

		// allocate memory if texture only has one mip tail per layer
		if ((!singleMipTail) && sparseMemoryRequirement.imageMipTailFirstLod < (uint32_t)info.mips)
		{
			VkMemoryAllocateInfo allocInfo;
			allocInfo.allocationSize = sparseMemoryRequirement.imageMipTailSize;
			allocInfo.memoryTypeIndex = memtype;

			VkDeviceMemory mem;
			vkAllocateMemory(dev, &allocInfo, nullptr, &mem);

			VkSparseMemoryBind sparseBind;
			sparseBind.resourceOffset = sparseMemoryRequirement.imageMipTailOffset + layer * sparseMemoryRequirement.imageMipTailStride;
			sparseBind.size = sparseMemoryRequirement.imageMipTailSize;
			sparseBind.memory = mem;

			// add to opaque bindings
			opaqueBinds.Append(sparseBind);
		}
	}

	if ((singleMipTail) && sparseMemoryRequirement.imageMipTailFirstLod < (uint32_t)info.mips)
	{
		VkMemoryAllocateInfo allocInfo;
		allocInfo.allocationSize = sparseMemoryRequirement.imageMipTailSize;
		allocInfo.memoryTypeIndex = memtype;

		VkDeviceMemory mem;
		vkAllocateMemory(dev, &allocInfo, nullptr, &mem);

		VkSparseMemoryBind sparseBind;
		sparseBind.resourceOffset = sparseMemoryRequirement.imageMipTailOffset;
		sparseBind.size = sparseMemoryRequirement.imageMipTailSize;
		sparseBind.memory = mem;

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
	mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;

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

	n_delete_array(sparseProperties);
	n_delete_array(sparseMemoryRequirements);

	CoreGraphics::SparseTextureId ret;
	ret.id8 = SparseTextureIdType;
	ret.id24 = id;
	return ret;
}

} // namespace CoreGraphics
