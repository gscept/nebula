#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan implementation of ResourceTable, ResourceTableLayout, and ResourcePipeline

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/resourcetable.h"
#include "vkloader.h"
#include <array>
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
	Resource table
*/
//------------------------------------------------------------------------------

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
const VkDescriptorSet& ResourceTableGetVkDescriptorSet(CoreGraphics::ResourceTableId id);
/// get set layout 
const VkDescriptorSetLayout& ResourceTableGetVkLayout(CoreGraphics::ResourceTableId id);

//------------------------------------------------------------------------------
/**
	Resource table layout
*/
//------------------------------------------------------------------------------
enum
{
	ResourceTableLayoutDevice,
	ResourceTableLayoutSetLayout,
	ResourceTableLayoutPoolSizes,
	ResourceTableLayoutSamplers,
	ResourceTableLayoutImmutableSamplerFlags,
	ResourceTableLayoutDescriptorPools,
	ResourceTableLayoutCurrentPool,
	ResourceTableLayoutPoolGrow
};
typedef Ids::IdAllocator<
	VkDevice,
	VkDescriptorSetLayout,
	Util::Array<VkDescriptorPoolSize>,
	Util::Array<Util::Pair<CoreGraphics::SamplerId, uint32_t>>,
	Util::HashTable<uint32_t, bool>,
	Util::Array<VkDescriptorPool>,
	VkDescriptorPool,
	uint32_t
> VkResourceTableLayoutAllocator;
extern VkResourceTableLayoutAllocator resourceTableLayoutAllocator;
extern VkDescriptorSetLayout emptySetLayout;

/// run this before using any resource tables
void SetupEmptyDescriptorSetLayout();

/// get table layout
const VkDescriptorSetLayout& ResourceTableLayoutGetVk(const CoreGraphics::ResourceTableLayoutId& id);
/// get current layout pool
const VkDescriptorPool& ResourceTableLayoutGetPool(const CoreGraphics::ResourceTableLayoutId& id);
/// request new layout pool for this layout
const VkDescriptorPool& ResourceTableLayoutNewPool(const CoreGraphics::ResourceTableLayoutId& id);


//------------------------------------------------------------------------------
/**
	Resource pipeline
*/
//------------------------------------------------------------------------------
typedef Ids::IdAllocator<
	VkDevice,
	VkPipelineLayout
> VkResourcePipelineAllocator;
extern VkResourcePipelineAllocator resourcePipelineAllocator;

/// get pipeline layout
const VkPipelineLayout& ResourcePipelineGetVk(const CoreGraphics::ResourcePipelineId& id);

} // namespace Vulkan
