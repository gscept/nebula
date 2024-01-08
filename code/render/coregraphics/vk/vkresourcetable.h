#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan implementation of ResourceTable, ResourceTableLayout, and ResourcePipeline

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/resourcetable.h"
#include "threading/spinlock.h"
#include <array>
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
    Resource table
*/
//------------------------------------------------------------------------------
enum class WriteType
{
    Image,
    Buffer,
    TexelBuffer,
    Tlas
};

struct WriteInfo
{
    union
    {
        VkDescriptorImageInfo img;
        VkDescriptorBufferInfo buf;
        VkBufferView tex;
        VkAccelerationStructureKHR tlas;
    };
    VkWriteDescriptorSet write;
    WriteType type;
};

enum
{
    ResourceTable_Device,
    ResourceTable_DescriptorSet,
    ResourceTable_DescriptorPoolIndex,
    ResourceTable_Lock,
    ResourceTable_Layout,
    ResourceTable_WriteInfos,
    ResourceTable_Copies
};

typedef Ids::IdAllocator<
    VkDevice,
    VkDescriptorSet,
    IndexT,
    Threading::Spinlock,
    CoreGraphics::ResourceTableLayoutId,
    Util::Array<WriteInfo>,
    Util::Array<VkCopyDescriptorSet>
> VkResourceTableAllocator;
extern VkResourceTableAllocator resourceTableAllocator;

/// Get descriptor set
const VkDescriptorSet& ResourceTableGetVkDescriptorSet(CoreGraphics::ResourceTableId id);
/// Get descriptor pool index
const IndexT ResourceTableGetVkPoolIndex(CoreGraphics::ResourceTableId id);
/// Get device used to create resource table
const VkDevice& ResourceTableGetVkDevice(CoreGraphics::ResourceTableId id);

//------------------------------------------------------------------------------
/**
    Resource table layout
*/
//------------------------------------------------------------------------------
enum
{
    ResourceTableLayout_Device,
    ResourceTableLayout_SetLayout,
    ResourceTableLayout_PoolSizes,
    ResourceTableLayout_Samplers,
    ResourceTableLayout_ImmutableSamplerFlags,
    ResourceTableLayout_DescriptorPools,
    ResourceTableLayout_DescriptorPoolFreeItems,
};
typedef Ids::IdAllocator<
    VkDevice,
    VkDescriptorSetLayout,
    Util::Array<VkDescriptorPoolSize>,
    Util::Array<Util::Pair<CoreGraphics::SamplerId, uint32_t>>,
    Util::HashTable<uint32_t, bool>,
    Util::Array<VkDescriptorPool>,
    Util::Array<uint>
> VkResourceTableLayoutAllocator;
extern VkResourceTableLayoutAllocator resourceTableLayoutAllocator;
extern VkDescriptorSetLayout EmptySetLayout;

/// run this before using any resource tables
void SetupEmptyDescriptorSetLayout();

/// get table layout
const VkDescriptorSetLayout& ResourceTableLayoutGetVk(const CoreGraphics::ResourceTableLayoutId& id);
/// allocate new descriptor set from pool
void ResourceTableLayoutAllocTable(const CoreGraphics::ResourceTableLayoutId& id, const VkDevice dev, uint overallocationSize, IndexT& outIndex, VkDescriptorSet& outSet);
/// deallocate descriptor set from pool
void ResourceTableLayoutDeallocTable(const CoreGraphics::ResourceTableLayoutId& id, const VkDevice dev, const VkDescriptorSet& set, const IndexT index);
/// Get descriptor pool
const VkDescriptorPool& ResourceTableLayoutGetVkDescriptorPool(const CoreGraphics::ResourceTableLayoutId& id, const IndexT index);
/// Get descriptor pool free items counter
uint* ResourceTableLayoutGetFreeItemsCounter(const CoreGraphics::ResourceTableLayoutId& id, const IndexT index);

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
