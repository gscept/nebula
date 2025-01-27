//------------------------------------------------------------------------------
//  vkresourcetable.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkresourcetable.h"
#include "coregraphics/resourcetable.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "vksampler.h"
#include "vktexture.h"
#include "vktextureview.h"
#include "vkbuffer.h"
#include "vkaccelerationstructure.h"

namespace Vulkan
{

VkResourceTableAllocator resourceTableAllocator;
VkResourceTableLayoutAllocator resourceTableLayoutAllocator;
VkResourcePipelineAllocator resourcePipelineAllocator;
VkDescriptorSetLayout EmptySetLayout;

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorSet&
ResourceTableGetVkDescriptorSet(CoreGraphics::ResourceTableId id)
{
    return resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
ResourceTableGetVkPoolIndex(CoreGraphics::ResourceTableId id)
{
    return resourceTableAllocator.Get<ResourceTable_DescriptorPoolIndex>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice&
ResourceTableGetVkDevice(CoreGraphics::ResourceTableId id)
{
    return resourceTableAllocator.Get<ResourceTable_Device>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
SetupEmptyDescriptorSetLayout()
{
    VkDescriptorSetLayoutCreateInfo info =
    {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        0,
        nullptr
    };
    VkResult res = vkCreateDescriptorSetLayout(Vulkan::GetCurrentDevice(), &info, nullptr, &EmptySetLayout);
    n_assert(res == VK_SUCCESS);

#if NEBULA_GRAPHICS_DEBUG
    VkDebugUtilsObjectNameInfoEXT dbgInfo =
    {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        nullptr,
        VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
        (uint64_t)EmptySetLayout,
        "Placeholder Resource Table Layout"
    };
    VkDevice dev = GetCurrentDevice();
    res = VkDebugObjectName(dev, &dbgInfo);
    n_assert(res == VK_SUCCESS);
#endif
}

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorSetLayout&
ResourceTableLayoutGetVk(const CoreGraphics::ResourceTableLayoutId& id)
{
    return resourceTableLayoutAllocator.Get<ResourceTableLayout_SetLayout>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableLayoutAllocTable(const CoreGraphics::ResourceTableLayoutId& id, const VkDevice dev, uint overallocationSize, IndexT& outIndex, VkDescriptorSet& outSet)
{
    Util::Array<uint32_t>& freeItems = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPoolFreeItems>(id.id);
    VkDescriptorPool pool = VK_NULL_HANDLE;
    for (IndexT i = 0; i < freeItems.Size(); i++)
    {
        // if free, return
        if (freeItems[i] > 0)
        {
            pool = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id)[i];
            outIndex = i;
            break;
        }
    }

    // no pool found, allocate new pool
    if (pool == VK_NULL_HANDLE)
    {
        Util::Array<VkDescriptorPoolSize> poolSizes = resourceTableLayoutAllocator.Get<ResourceTableLayout_PoolSizes>(id.id);
        for (IndexT i = 0; i < poolSizes.Size(); i++)
        {
            poolSizes[i].descriptorCount *= overallocationSize;
        }

        // create new pool
        VkDescriptorPoolCreateInfo poolInfo =
        {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            nullptr,
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            overallocationSize,
            (uint32_t)poolSizes.Size(),
            poolSizes.Size() > 0 ? poolSizes.Begin() : nullptr
        };

        VkResult res = vkCreateDescriptorPool(dev, &poolInfo, nullptr, &pool);
        n_assert(res == VK_SUCCESS);

        // add to list of pools, and set new pointer to the new pool
        resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id).Append(pool);
        freeItems.Append(overallocationSize);
        outIndex = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id).Size() - 1;
    }

    VkDescriptorSetAllocateInfo dsetAlloc =
    {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        pool,
        1,
        &ResourceTableLayoutGetVk(id)
    };

    VkResult res = vkAllocateDescriptorSets(dev, &dsetAlloc, &outSet);
    n_assert(res == VK_SUCCESS);

    // decrement the used descriptor counter
    freeItems[outIndex]--;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableLayoutDeallocTable(const CoreGraphics::ResourceTableLayoutId& id, const VkDevice dev, const VkDescriptorSet& set, const IndexT index)
{
    VkDescriptorPool& pool = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id)[index];
    VkResult res = vkFreeDescriptorSets(dev, pool, 1, &set);
    n_assert(res == VK_SUCCESS);
    resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPoolFreeItems>(id.id)[index]++;
}

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorPool&
ResourceTableLayoutGetVkDescriptorPool(const CoreGraphics::ResourceTableLayoutId& id, const IndexT index)
{
    return resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id)[index];
}

//------------------------------------------------------------------------------
/**
*/
uint*
ResourceTableLayoutGetFreeItemsCounter(const CoreGraphics::ResourceTableLayoutId& id, const IndexT index)
{
    Util::Array<uint>& freeItems = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPoolFreeItems>(id.id);
    return &freeItems[index];
}

//------------------------------------------------------------------------------
/**
*/
const VkPipelineLayout&
ResourcePipelineGetVk(const CoreGraphics::ResourcePipelineId& id)
{
    return resourcePipelineAllocator.Get<1>(id.id);
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

Util::Array<CoreGraphics::ResourceTableId> PendingTableCommits;
bool ResourceTableBlocked = false;
Threading::CriticalSection PendingTableCommitsLock;

//------------------------------------------------------------------------------
/**
*/
ResourceTableId
CreateResourceTable(const ResourceTableCreateInfo& info)
{
    Ids::Id32 id = resourceTableAllocator.Alloc();

    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id);
    IndexT& poolIndex = resourceTableAllocator.Get<ResourceTable_DescriptorPoolIndex>(id);
    CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<ResourceTable_Layout>(id);

    dev = Vulkan::GetCurrentDevice();
    layout = info.layout;
    ResourceTableLayoutAllocTable(layout, dev, info.overallocationSize, poolIndex, set);

    ResourceTableId ret = id;
    if (info.name != nullptr)
        ObjectSetName(ret, info.name);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourceTable(const ResourceTableId id)
{
    n_assert(id != InvalidResourceTableId);
    CoreGraphics::DelayedDeleteDescriptorSet(id);

    resourceTableAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const ResourceTableLayoutId&
ResourceTableGetLayout(ResourceTableId id)
{
    return resourceTableAllocator.Get<ResourceTable_Layout>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableCopy(const ResourceTableId from, IndexT fromSlot, IndexT fromIndex, const ResourceTableId to, IndexT toSlot, IndexT toIndex, const SizeT numResources)
{
    VkDescriptorSet& fromSet = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(from.id);
    VkDescriptorSet& toSet = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(to.id);

    Threading::SpinlockScope scope1(&resourceTableAllocator.Get<ResourceTable_Lock>(from.id));
    Threading::SpinlockScope scope2(&resourceTableAllocator.Get<ResourceTable_Lock>(to.id));
    Util::Array<VkCopyDescriptorSet, 4>& copies = resourceTableAllocator.Get<ResourceTable_Copies>(to.id);

    VkCopyDescriptorSet copy =
    {
        VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
        nullptr,
        fromSet,
        (uint32_t)fromSlot,
        (uint32_t)fromIndex,
        toSet,
        (uint32_t)toSlot,
        (uint32_t)toIndex,
        (uint32_t)numResources
    };
    copies.Append(copy);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetTexture(const ResourceTableId id, const ResourceTableTexture& tex)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<ResourceTable_Layout>(id.id);
    const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayout_ImmutableSamplerFlags>(layout.id);

    VkDescriptorImageInfo img;
    if (immutable[tex.slot])
    {
        n_assert(tex.sampler == InvalidSamplerId);
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        img.sampler = VK_NULL_HANDLE;
    }
    else
    {
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        img.sampler = tex.sampler == InvalidSamplerId ? VK_NULL_HANDLE : SamplerGetVk(tex.sampler);
    }

    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (tex.tex == InvalidTextureId)
        img.imageView = VK_NULL_HANDLE;
    else if (tex.isStencil)
        img.imageView = TextureGetVkStencilImageView(tex.tex);
    else
        img.imageView = TextureGetVkImageView(tex.tex);

    WriteInfo inf;
    inf.img = img;
    inf.write = write;
    inf.type = WriteType::Image;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetTexture(const ResourceTableId id, const ResourceTableTextureView& tex)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<ResourceTable_Layout>(id.id);
    const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayout_ImmutableSamplerFlags>(layout.id);

    VkDescriptorImageInfo img;
    if (immutable[tex.slot])
    {
        n_assert(tex.sampler == InvalidSamplerId);
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        img.sampler = VK_NULL_HANDLE;
    }
    else
    {
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        img.sampler = tex.sampler == InvalidSamplerId ? VK_NULL_HANDLE : SamplerGetVk(tex.sampler);
    }

    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (tex.tex == InvalidTextureViewId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureViewGetVk(tex.tex);

    WriteInfo inf;
    inf.img = img;
    inf.write = write;
    inf.type = WriteType::Image;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetInputAttachment(const ResourceTableId id, const ResourceTableInputAttachment& tex)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    VkDescriptorImageInfo img;
    img.sampler = VK_NULL_HANDLE;
    img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (tex.tex == InvalidTextureViewId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureViewGetVk(tex.tex);

    WriteInfo inf;
    inf.img = img;
    inf.write = write;
    inf.type = WriteType::Image;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetRWTexture(const ResourceTableId id, const ResourceTableTexture& tex)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    VkDescriptorImageInfo img;
    img.sampler = VK_NULL_HANDLE;
    img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    if (tex.tex == InvalidTextureId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureGetVkImageView(tex.tex);

    WriteInfo inf;
    inf.img = img;
    inf.write = write;
    inf.type = WriteType::Image;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetRWTexture(const ResourceTableId id, const ResourceTableTextureView& tex)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    VkDescriptorImageInfo img;
    img.sampler = VK_NULL_HANDLE;
    img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    if (tex.tex == InvalidTextureViewId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureViewGetVk(tex.tex);

    WriteInfo inf;
    inf.img = img;
    inf.write = write;
    inf.type = WriteType::Image;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetConstantBuffer(const ResourceTableId id, const ResourceTableBuffer& buf)
{
    n_assert(!buf.texelBuffer);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(buf.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    if (buf.dynamicOffset)
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    else if (buf.texelBuffer)
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    else
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.dstArrayElement = buf.index;
    write.dstBinding = buf.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    n_assert2(write.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, "Texel buffers are not implemented");

    VkDescriptorBufferInfo buff;
    if (buf.buf == InvalidBufferId)
        buff.buffer = VK_NULL_HANDLE;
    else
        buff.buffer = BufferGetVk(buf.buf);
    buff.offset = buf.offset;
    buff.range = buf.size == NEBULA_WHOLE_BUFFER_SIZE ? Math::min(BufferGetByteSize(buf.buf), CoreGraphics::MaxConstantBufferSize) : buf.size;

    WriteInfo inf;
    inf.buf = buff;
    inf.write = write;
    inf.type = WriteType::Buffer;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetRWBuffer(const ResourceTableId id, const ResourceTableBuffer& buf)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(buf.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    if (buf.dynamicOffset)
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    else if (buf.texelBuffer)
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    else
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.dstArrayElement = buf.index;
    write.dstBinding = buf.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    n_assert2(write.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, "Texel buffers are not implemented");

    VkDescriptorBufferInfo buff;
    if (buf.buf == InvalidBufferId)
        buff.buffer = VK_NULL_HANDLE;
    else
        buff.buffer = BufferGetVk(buf.buf);
    buff.offset = buf.offset;
    buff.range = buf.size == NEBULA_WHOLE_BUFFER_SIZE ? VK_WHOLE_SIZE : buf.size;
    WriteInfo inf;
    inf.buf = buff;
    inf.write = write;
    inf.type = WriteType::Buffer;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetSampler(const ResourceTableId id, const ResourceTableSampler& samp)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(samp.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.descriptorCount = 1;
    write.dstArrayElement = 0;
    write.dstBinding = samp.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    VkDescriptorImageInfo img;
    if (samp.samp == InvalidSamplerId)
        img.sampler = VK_NULL_HANDLE;
    else
        img.sampler = SamplerGetVk(samp.samp);

    img.imageView = VK_NULL_HANDLE;
    img.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    WriteInfo inf;
    inf.img = img;
    inf.write = write;
    inf.type = WriteType::Image;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetAccelerationStructure(const ResourceTableId id, const ResourceTableTlas& tlas)
{
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id);

    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);

    n_assert(tlas.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write.descriptorCount = 1;
    write.dstArrayElement = 0;
    write.dstBinding = tlas.slot;
    write.dstSet = set;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    WriteInfo inf;
    inf.tlas = Vulkan::TlasGetVk(tlas.tlas);
    inf.write = write;
    inf.type = WriteType::Tlas;
    infoList.Append(inf);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableBlock(bool b)
{
    ResourceTableBlocked = b;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableCommitChanges(const ResourceTableId id)
{
    n_assert(!ResourceTableBlocked);

    // resource tables are blocked, add to pending write queue
    Threading::SpinlockScope scope(&resourceTableAllocator.Get<ResourceTable_Lock>(id.id));
    Util::Array<WriteInfo, 16>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id);
    Util::Array<VkCopyDescriptorSet, 4>& copies = resourceTableAllocator.Get<ResourceTable_Copies>(id.id);
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id);

    // because we store the write-infos in the other list, and the VkWriteDescriptorSet wants a pointer to the structure
    // we need to re-assign the pointers, but thankfully they have values from before
    IndexT i;
    for (i = 0; i < infoList.Size(); i++)
    {
        switch (infoList[i].type)
        {
            case WriteType::Image:
                infoList[i].write.pImageInfo = &infoList[i].img;
                break;
            case WriteType::Buffer:
                infoList[i].write.pBufferInfo = &infoList[i].buf;
                break;
            case WriteType::TexelBuffer:
                infoList[i].write.pTexelBufferView = &infoList[i].tex;
                break;
            case WriteType::Tlas:
            {
                infoList[i].tlasWrite = VkWriteDescriptorSetAccelerationStructureKHR
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                    .pNext = nullptr,
                    .accelerationStructureCount = 1,
                    .pAccelerationStructures = &(infoList[i].tlas)
                };
                infoList[i].write.pNext = &infoList[i].tlasWrite;
                break;
            }
        }
        vkUpdateDescriptorSets(dev, 1, &infoList[i].write, 0, nullptr);
    }
    infoList.Free();

    // Do copies
    if (copies.Size() > 0)
    {
        vkUpdateDescriptorSets(dev, 0, nullptr, copies.Size(), copies.Begin());
        copies.Free();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AddBinding(Util::HashTable<uint32_t, VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutBinding& binding)
{
    IndexT index = bindings.FindIndex(binding.binding);
    if (index != InvalidIndex)
    {
        const VkDescriptorSetLayoutBinding& prevBinding = bindings.ValueAtIndex(binding.binding, index);
        if (prevBinding.descriptorCount != binding.descriptorCount
            || prevBinding.descriptorType != binding.descriptorType
            || prevBinding.pImmutableSamplers != binding.pImmutableSamplers
            || prevBinding.stageFlags != binding.stageFlags)
        {
            n_error("ResourceTable: Incompatible aliasing in for binding %d", binding.binding);
        }
    }
    else
        bindings.Add(binding.binding, binding);
}

//------------------------------------------------------------------------------
/**
*/
void
CountDescriptors(Util::HashTable<uint32_t, uint32_t>& counters, const VkDescriptorSetLayoutBinding& binding)
{
    // accumulate textures for same bind point
    IndexT countIndex = counters.FindIndex(binding.binding);
    if (countIndex == InvalidIndex)
        counters.Add(binding.binding, binding.descriptorCount);
    else
    {
        uint32_t& count = counters.ValueAtIndex(binding.binding, countIndex);
        count = Math::max(count, (uint32_t)binding.descriptorCount);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SetupDescriptorSize(VkDescriptorPoolSize& size, Util::HashTable<uint32_t, uint32_t>& counter, Util::Array<VkDescriptorPoolSize>& outSizes)
{
    auto it = counter.Begin();
    while (it != counter.End())
    {
        size.descriptorCount += *it.val;
        it++;
    }
    if (size.descriptorCount > 0)
        outSizes.Append(size);
}

//------------------------------------------------------------------------------
/**
*/
ResourceTableLayoutId
CreateResourceTableLayout(const ResourceTableLayoutCreateInfo& info)
{
    Ids::Id32 id = resourceTableLayoutAllocator.Alloc();

    VkDevice& dev = resourceTableLayoutAllocator.Get<ResourceTableLayout_Device>(id);
    VkDescriptorSetLayout& layout = resourceTableLayoutAllocator.Get<ResourceTableLayout_SetLayout>(id);
    Util::Array<Util::Pair<CoreGraphics::SamplerId, uint32_t>>& samplers = resourceTableLayoutAllocator.Get<ResourceTableLayout_Samplers>(id);
    Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayout_ImmutableSamplerFlags>(id);
    Util::Array<VkDescriptorPoolSize>& poolSizes = resourceTableLayoutAllocator.Get<ResourceTableLayout_PoolSizes>(id);

    dev = Vulkan::GetCurrentDevice();
    Util::HashTable<uint32_t, VkDescriptorSetLayoutBinding> bindings;

    //------------------------------------------------------------------------------
    /**
        Textures and Texture-Sampler pairs
    */
    //------------------------------------------------------------------------------

    VkDescriptorPoolSize sampledImageSize, combinedImageSize;
    sampledImageSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sampledImageSize.descriptorCount = 0;
    combinedImageSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    combinedImageSize.descriptorCount = 0;

    Util::HashTable<uint32_t, uint32_t> textureCount;
    Util::HashTable<uint32_t, uint32_t> sampledTextureCount;
    Util::HashTable<uint32_t, uint32_t> imageCount;
    Util::HashTable<uint32_t, uint32_t> uniformCount;
    Util::HashTable<uint32_t, uint32_t> dynamicUniformCount;
    Util::HashTable<uint32_t, uint32_t> bufferCount;
    Util::HashTable<uint32_t, uint32_t> dynamicBufferCount;

    // setup textures, or texture-sampler pairs
    for (IndexT i = 0; i < info.textures.Size(); i++)
    {
        const ResourceTableLayoutTexture& tex = info.textures[i];
        n_assert(tex.num >= 0);
        VkDescriptorSetLayoutBinding binding;
        binding.binding = tex.slot;
        binding.descriptorCount = tex.num;
        if (tex.immutableSampler == InvalidSamplerId)
        {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            binding.pImmutableSamplers = nullptr;
            if (!immutable.Contains(tex.slot))
                immutable.Add(tex.slot, false);

            CountDescriptors(textureCount, binding);
        }
        else
        {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.pImmutableSamplers = &SamplerGetVk(tex.immutableSampler);
            if (!immutable.Contains(tex.slot))
                immutable.Add(tex.slot, true);

            CountDescriptors(sampledTextureCount, binding);
        }
        binding.stageFlags = VkTypes::AsVkShaderVisibility(tex.visibility);

        AddBinding(bindings, binding);
    }

    // update pool sizes
    SetupDescriptorSize(sampledImageSize, textureCount, poolSizes);
    SetupDescriptorSize(combinedImageSize, sampledTextureCount, poolSizes);

    //------------------------------------------------------------------------------
    /**
        RW texture
    */
    //------------------------------------------------------------------------------

    VkDescriptorPoolSize rwImageSize;
    rwImageSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    rwImageSize.descriptorCount = 0;

    // setup readwrite textures
    for (IndexT i = 0; i < info.rwTextures.Size(); i++)
    {
        const ResourceTableLayoutTexture& tex = info.rwTextures[i];
        n_assert(tex.num >= 0);
        n_assert(tex.immutableSampler == InvalidSamplerId);
        VkDescriptorSetLayoutBinding binding;
        binding.binding = tex.slot;
        binding.descriptorCount = tex.num;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = VkTypes::AsVkShaderVisibility(tex.visibility);
        AddBinding(bindings, binding);

        CountDescriptors(imageCount, binding);
    }

    SetupDescriptorSize(rwImageSize, imageCount, poolSizes);

    //------------------------------------------------------------------------------
    /**
        Constant buffers
    */
    //------------------------------------------------------------------------------

    VkDescriptorPoolSize cbSize, cbDynamicSize;
    cbSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cbSize.descriptorCount = 0;
    cbDynamicSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    cbDynamicSize.descriptorCount = 0;

    // setup constant buffers
    for (IndexT i = 0; i < info.constantBuffers.Size(); i++)
    {
        const ResourceTableLayoutConstantBuffer& buf = info.constantBuffers[i];
        n_assert(buf.num >= 0);
        VkDescriptorSetLayoutBinding binding;
        binding.binding = buf.slot;
        binding.descriptorCount = buf.num;
        binding.descriptorType = buf.dynamicOffset ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = VkTypes::AsVkShaderVisibility(buf.visibility);
        AddBinding(bindings, binding);

        CountDescriptors(buf.dynamicOffset ? dynamicUniformCount : uniformCount, binding);
    }

    // update pool sizes
    SetupDescriptorSize(cbDynamicSize, dynamicUniformCount, poolSizes);
    SetupDescriptorSize(cbSize, uniformCount, poolSizes);

    //------------------------------------------------------------------------------
    /**
        RW buffers
    */
    //------------------------------------------------------------------------------

    VkDescriptorPoolSize rwBufferSize, rwDynamicBufferSize;
    rwBufferSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    rwBufferSize.descriptorCount = 0;
    rwDynamicBufferSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    rwDynamicBufferSize.descriptorCount = 0;

    // setup readwrite buffers
    for (IndexT i = 0; i < info.rwBuffers.Size(); i++)
    {
        const ResourceTableLayoutShaderRWBuffer& buf = info.rwBuffers[i];
        n_assert(buf.num >= 0);
        VkDescriptorSetLayoutBinding binding;
        binding.binding = buf.slot;
        binding.descriptorCount = buf.num;
        binding.descriptorType = buf.dynamicOffset ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = VkTypes::AsVkShaderVisibility(buf.visibility);
        AddBinding(bindings, binding);

        CountDescriptors(buf.dynamicOffset ? dynamicBufferCount : bufferCount, binding);
    }

    // update pool sizes
    SetupDescriptorSize(rwDynamicBufferSize, dynamicBufferCount, poolSizes);
    SetupDescriptorSize(rwBufferSize, bufferCount, poolSizes);

    //------------------------------------------------------------------------------
    /**
        Acceleration structures
    */
    //------------------------------------------------------------------------------
    if (CoreGraphics::RayTracingSupported)
    {
        VkDescriptorPoolSize accelerationStructureSize;
        accelerationStructureSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureSize.descriptorCount = 0;

        // Setup acceleration structures
        for (IndexT i = 0; i < info.accelerationStructures.Size(); i++)
        {
            const ResourceTableLayoutAccelerationStructure& tlas = info.accelerationStructures[i];
            VkDescriptorSetLayoutBinding binding;
            binding.binding = tlas.slot;
            binding.descriptorCount = tlas.num;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            binding.stageFlags = VkTypes::AsVkShaderVisibility(tlas.visibility);
            AddBinding(bindings, binding);

            accelerationStructureSize.descriptorCount += tlas.num;
        }

        if (accelerationStructureSize.descriptorCount > 0)
            poolSizes.Append(accelerationStructureSize);
    }

    //------------------------------------------------------------------------------
    /**
        Samplers
    */
    //------------------------------------------------------------------------------

    VkDescriptorPoolSize samplerSize;
    samplerSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerSize.descriptorCount = 0;

    // setup sampler objects
    for (IndexT i = 0; i < info.samplers.Size(); i++)
    {
        const ResourceTableLayoutSampler& samp = info.samplers[i];
        VkDescriptorSetLayoutBinding binding;
        binding.binding = samp.slot;
        binding.descriptorCount = 1;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        binding.pImmutableSamplers = &SamplerGetVk(samp.sampler);
        binding.stageFlags = VkTypes::AsVkShaderVisibility(samp.visibility);
        AddBinding(bindings, binding);

        // add static samplers
        samplers.Append(Util::MakePair(samp.sampler, samp.slot));
        samplerSize.descriptorCount++;
    }

    if (samplerSize.descriptorCount > 0)
        poolSizes.Append(samplerSize);


    //------------------------------------------------------------------------------
    /**
        Input attachments
    */
    //------------------------------------------------------------------------------

    VkDescriptorPoolSize inputAttachmentSize;
    inputAttachmentSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    inputAttachmentSize.descriptorCount = 0;

    // setup input attachments
    for (IndexT i = 0; i < info.inputAttachments.Size(); i++)
    {
        const ResourceTableLayoutInputAttachment& tex = info.inputAttachments[i];
        n_assert(tex.num >= 0);
        VkDescriptorSetLayoutBinding binding;
        binding.binding = tex.slot;
        binding.descriptorCount = tex.num;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = VkTypes::AsVkShaderVisibility(tex.visibility);
        AddBinding(bindings, binding);

        inputAttachmentSize.descriptorCount += tex.num;
    }

    if (inputAttachmentSize.descriptorCount > 0)
        poolSizes.Append(inputAttachmentSize);


    // Create layout
    if (bindings.Size() > 0)
    {
        Util::FixedArray<VkDescriptorBindingFlags> flags(bindings.Size());
        flags.Fill(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags =
        {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            nullptr,
            (uint32_t)flags.Size(),
            flags.Begin()
        };
        auto binds = bindings.ValuesAsArray();
        VkDescriptorSetLayoutCreateInfo dslInfo =
        {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            &bindingFlags,
            0,                                                              // USE vkCmdPushDescriptorSetKHR IN THE FUTURE!
            (uint32_t)bindings.Size(),
            binds.Begin()
        };
        VkResult res = vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &layout);
        n_assert(res == VK_SUCCESS);
    }

    ResourceTableLayoutId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourceTableLayout(const ResourceTableLayoutId& id)
{
    VkDevice& dev = resourceTableLayoutAllocator.Get<ResourceTableLayout_Device>(id.id);
    VkDescriptorSetLayout& layout = resourceTableLayoutAllocator.Get<ResourceTableLayout_SetLayout>(id.id);
    vkDestroyDescriptorSetLayout(dev, layout, nullptr);

    // destroy all pools
    Util::Array<VkDescriptorPool>& pools = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id);
    for (IndexT i = 0; i < pools.Size(); i++)
        vkDestroyDescriptorPool(dev, pools[i], nullptr);

    pools.Clear();

    resourceTableLayoutAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
ResourcePipelineId
CreateResourcePipeline(const ResourcePipelineCreateInfo& info)
{
    Ids::Id32 id = resourcePipelineAllocator.Alloc();

    VkDevice& dev = resourcePipelineAllocator.Get<0>(id);
    VkPipelineLayout& layout = resourcePipelineAllocator.Get<1>(id);
    dev = Vulkan::GetCurrentDevice();

    Util::Array<VkDescriptorSetLayout> layouts;

    IndexT i;
    for (i = 0; i < info.indices.Size(); i++)
    {
        while (info.indices[i] != layouts.Size())
        {
            layouts.Append(EmptySetLayout);
        }
        layouts.Append(resourceTableLayoutAllocator.Get<ResourceTableLayout_SetLayout>(info.tables[i].id));
    }

    VkPushConstantRange push;
    push.size = info.push.size;
    push.offset = info.push.offset;
    push.stageFlags = VkTypes::AsVkShaderVisibility(info.push.vis);

    VkPipelineLayoutCreateInfo crInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        (uint32_t)layouts.Size(),
        layouts.Begin(),
        push.size > 0 ? 1u : 0u,
        &push
    };
    VkResult res = vkCreatePipelineLayout(dev, &crInfo, nullptr, &layout);
    n_assert(res == VK_SUCCESS);

    ResourcePipelineId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourcePipeline(const ResourcePipelineId& id)
{
    VkDevice& dev = resourcePipelineAllocator.Get<0>(id.id);
    VkPipelineLayout& layout = resourcePipelineAllocator.Get<1>(id.id);
    vkDestroyPipelineLayout(dev, layout, nullptr);

    resourcePipelineAllocator.Dealloc(id.id);
}

} // namespace CoreGraphics
