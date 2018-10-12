#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan implementation of ResourceTable

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/resourcetable.h"
#include <vulkan/vulkan.h>
namespace Vulkan
{

union WriteInfo
{
	VkDescriptorImageInfo img;
	VkDescriptorBufferInfo buf;
	VkBufferView tex;
};

typedef Ids::IdAllocator<
	VkDevice,
	VkDescriptorSet,
	VkDescriptorPool,
	CoreGraphics::ResourceTableLayoutId,
	Util::Array<VkWriteDescriptorSet>,
	Util::Array<WriteInfo>	
> VkResourceTableAllocator;
extern VkResourceTableAllocator resourceTableAllocator;

/// get descriptor set
const VkDescriptorSet& ResourceTableGetVkDescriptorSet(const CoreGraphics::ResourceTableId& id);
/// get set layout 
const VkDescriptorSetLayout& ResourceTableGetVkLayout(const CoreGraphics::ResourceTableId& id);

typedef Ids::IdAllocator<
	VkDevice,
	VkDescriptorSetLayout,
	Util::Array<std::pair<CoreGraphics::SamplerId, uint32_t>>,
	Util::HashTable<uint32_t, bool>
> VkResourceTableLayoutAllocator;
extern VkResourceTableLayoutAllocator resourceTableLayoutAllocator;
extern VkDescriptorSetLayout emptySetLayout;

/// run this before using any resource tables
void SetupEmptyDescriptorSet();

/// get table layout
const VkDescriptorSetLayout& ResourceTableLayoutGetVk(const CoreGraphics::ResourceTableLayoutId& id);

typedef Ids::IdAllocator<
	VkDevice,
	VkPipelineLayout
> VkResourcePipelineAllocator;
extern VkResourcePipelineAllocator resourcePipelineAllocator;

/// get pipeline layout
const VkPipelineLayout& ResourcePipelineGetVk(const CoreGraphics::ResourcePipelineId& id);

} // namespace Vulkan
