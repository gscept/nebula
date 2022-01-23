//------------------------------------------------------------------------------
//  vkresourcetable.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkresourcetable.h"
#include "coregraphics/resourcetable.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "vksampler.h"
#include "vktexture.h"
#include "vktextureview.h"
#include "vkbuffer.h"

namespace Vulkan
{

VkResourceTableAllocator resourceTableAllocator;
VkResourceTableLayoutAllocator resourceTableLayoutAllocator;
VkResourcePipelineAllocator resourcePipelineAllocator;
VkDescriptorSetLayout emptySetLayout;

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorSet&
ResourceTableGetVkDescriptorSet(CoreGraphics::ResourceTableId id)
{
    return resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorSetLayout&
ResourceTableGetVkLayout(CoreGraphics::ResourceTableId id)
{
    return ResourceTableLayoutGetVk(resourceTableAllocator.Get<ResourceTable_Layout>(id.id24));
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
    VkResult res = vkCreateDescriptorSetLayout(Vulkan::GetCurrentDevice(), &info, nullptr, &emptySetLayout);
    n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorSetLayout&
ResourceTableLayoutGetVk(const CoreGraphics::ResourceTableLayoutId& id)
{
    return resourceTableLayoutAllocator.Get<ResourceTableLayout_SetLayout>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableLayoutAllocTable(const CoreGraphics::ResourceTableLayoutId& id, const VkDevice dev, uint overallocationSize, IndexT& outIndex, VkDescriptorSet& outSet)
{
    Util::Array<uint32_t>& freeItems = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPoolFreeItems>(id.id24);
    VkDescriptorPool pool = VK_NULL_HANDLE;
    for (IndexT i = 0; i < freeItems.Size(); i++)
    {
        // if free, return
        if (freeItems[i] > 0)
        {
            pool = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id24)[i];
            outIndex = i;
            break;
        }
    }

    // no pool found, allocate new pool
    if (pool == VK_NULL_HANDLE)
    {
        Util::Array<VkDescriptorPoolSize> poolSizes = resourceTableLayoutAllocator.Get<ResourceTableLayout_PoolSizes>(id.id24);
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
        resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id24).Append(pool);
        freeItems.Append(overallocationSize);
        outIndex = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id24).Size() - 1;
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
    VkDescriptorPool& pool = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id24)[index];
    vkFreeDescriptorSets(dev, pool, 1, &set);
    resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPoolFreeItems>(id.id24)[index]++;
}

//------------------------------------------------------------------------------
/**
*/
const VkPipelineLayout&
ResourcePipelineGetVk(const CoreGraphics::ResourcePipelineId& id)
{
    return resourcePipelineAllocator.Get<1>(id.id24);
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

Util::Array<CoreGraphics::ResourceTableId> PendingTableCommits;
bool ResourceTableBlocked = true;
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

    ResourceTableId ret;
    ret.id24 = id;
    ret.id8 = ResourceTableIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourceTable(const ResourceTableId id)
{
    n_assert(id != InvalidResourceTableId);
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<ResourceTable_Layout>(id.id24);
    const IndexT& poolIndex = resourceTableAllocator.Get<ResourceTable_DescriptorPoolIndex>(id.id24);
    const VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);
    ResourceTableLayoutDeallocTable(layout, dev, set, poolIndex);

    resourceTableAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableCopy(const ResourceTableId from, IndexT fromSlot, IndexT fromIndex, const ResourceTableId to, IndexT toSlot, IndexT toIndex, const SizeT numResources)
{
    VkDescriptorSet& fromSet = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(from.id24);
    VkDescriptorSet& toSet = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(to.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(to.id24);
    Threading::AssertingScope scope(&mutex);

    Util::Array<VkCopyDescriptorSet>& copies = resourceTableAllocator.Get<ResourceTable_Copies>(to.id24);

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
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<ResourceTable_Layout>(id.id24);
    const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayout_ImmutableSamplerFlags>(layout.id24);

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
    img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (tex.tex == InvalidTextureId)
        img.imageView = VK_NULL_HANDLE;
    else if (tex.isStencil)
        img.imageView = TextureGetVkStencilImageView(tex.tex);
    else
        img.imageView = TextureGetVkImageView(tex.tex);

    WriteInfo inf;
    inf.img = img;
    infoList.Append(inf);

    write.pImageInfo = &img;            // this is just provisionary, it will go out of scope immediately, but it wont be null!
    write.pTexelBufferView = nullptr;
    write.pBufferInfo = nullptr;

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetTexture(const ResourceTableId id, const ResourceTableTextureView& tex)
{
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<ResourceTable_Layout>(id.id24);
    const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayout_ImmutableSamplerFlags>(layout.id24);

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
    img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (tex.tex == InvalidTextureViewId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureViewGetVk(tex.tex);

    WriteInfo inf;
    inf.img = img;
    infoList.Append(inf);

    write.pImageInfo = &img;            // this is just provisionary, it will go out of scope immediately, but it wont be null!
    write.pTexelBufferView = nullptr;
    write.pBufferInfo = nullptr;

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetInputAttachment(const ResourceTableId id, const ResourceTableInputAttachment& tex)
{
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;

    VkDescriptorImageInfo img;
    img.sampler = VK_NULL_HANDLE;
    img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    if (tex.tex == InvalidTextureViewId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureViewGetVk(tex.tex);

    WriteInfo inf;
    inf.img = img;
    infoList.Append(inf);

    write.pImageInfo = &img;            // this is just provisionary, it will go out of scope immediately, but it wont be null!
    write.pTexelBufferView = nullptr;
    write.pBufferInfo = nullptr;

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetRWTexture(const ResourceTableId id, const ResourceTableTexture& tex)
{
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;

    VkDescriptorImageInfo img;
    img.sampler = VK_NULL_HANDLE;
    img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    if (tex.tex == InvalidTextureId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureGetVkImageView(tex.tex);

    WriteInfo inf;
    inf.img = img;
    infoList.Append(inf);

    write.pImageInfo = &img;            // this is just provisionary, it will go out of scope immediately, but it wont be null!
    write.pTexelBufferView = nullptr;
    write.pBufferInfo = nullptr;

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetRWTexture(const ResourceTableId id, const ResourceTableTextureView& tex)
{
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

    n_assert(tex.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.descriptorCount = 1;
    write.dstArrayElement = tex.index;
    write.dstBinding = tex.slot;
    write.dstSet = set;

    VkDescriptorImageInfo img;
    img.sampler = VK_NULL_HANDLE;
    img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    if (tex.tex == InvalidTextureViewId)
        img.imageView = VK_NULL_HANDLE;
    else
        img.imageView = TextureViewGetVk(tex.tex);

    WriteInfo inf;
    inf.img = img;
    infoList.Append(inf);

    write.pImageInfo = &img;            // this is just provisionary, it will go out of scope immediately, but it wont be null!
    write.pTexelBufferView = nullptr;
    write.pBufferInfo = nullptr;

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetConstantBuffer(const ResourceTableId id, const ResourceTableBuffer& buf)
{
    n_assert(!buf.texelBuffer);
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

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

    n_assert2(write.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, "Texel buffers are not implemented");

    VkDescriptorBufferInfo buff;
    if (buf.buf == InvalidBufferId)
        buff.buffer = VK_NULL_HANDLE;
    else
        buff.buffer = BufferGetVk(buf.buf);
    buff.offset = buf.offset;
    buff.range = buf.size == NEBULA_WHOLE_BUFFER_SIZE ? VK_WHOLE_SIZE : buf.size;

    WriteInfo inf;
    inf.buf = buff;
    infoList.Append(inf);

    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;
    write.pBufferInfo = &buff;          // this is just provisionary, it will go out of scope immediately, but it wont be null!

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetRWBuffer(const ResourceTableId id, const ResourceTableBuffer& buf)
{
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

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
    infoList.Append(inf);

    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;
    write.pBufferInfo = &buff;          // this is just provisionary, it will go out of scope immediately, but it wont be null!

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetSampler(const ResourceTableId id, const ResourceTableSampler& samp)
{
    VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);
    VkDescriptorSet& set = resourceTableAllocator.Get<ResourceTable_DescriptorSet>(id.id24);

    Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
    Threading::AssertingScope scope(&mutex);
    Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
    Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);

    n_assert(samp.slot != InvalidIndex);

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    write.descriptorCount = 1;
    write.dstArrayElement = 0;
    write.dstBinding = samp.slot;
    write.dstSet = set;

    VkDescriptorImageInfo img;
    if (samp.samp == InvalidSamplerId)
        img.sampler = VK_NULL_HANDLE;
    else
        img.sampler = SamplerGetVk(samp.samp);

    img.imageView = VK_NULL_HANDLE;
    img.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    WriteInfo inf;
    inf.img = img;
    infoList.Append(inf);

    write.pImageInfo = &img; // this is just provisionary, it will go out of scope immediately, but it wont be null!
    write.pBufferInfo = nullptr;
    write.pTexelBufferView = nullptr;

    writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableBlock(bool b)
{
    bool wasUnblocked = ResourceTableBlocked && !b;
    ResourceTableBlocked = b;

    if (wasUnblocked)
    {
        // if we were blocked but aren't anymore, make sure to flush any pending resource tables
        PendingTableCommitsLock.Enter();
        for (ResourceTableId& table : PendingTableCommits)
            ResourceTableCommitChanges(table);

        PendingTableCommits.Clear();
        PendingTableCommitsLock.Leave();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableCommitChanges(const ResourceTableId id)
{
    // resource tables are blocked, add to pending write queue
    if (ResourceTableBlocked)
    {
        PendingTableCommitsLock.Enter();
        PendingTableCommits.Append(id);
        PendingTableCommitsLock.Leave();
    }
    else
    {
        Threading::AssertingMutex& mutex = resourceTableAllocator.Get<ResourceTable_AssertingMutex>(id.id24);
        Threading::AssertingScope scope(&mutex);

        Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<ResourceTable_Writes>(id.id24);
        Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<ResourceTable_WriteInfos>(id.id24);
        Util::Array<VkCopyDescriptorSet>& copies = resourceTableAllocator.Get<ResourceTable_Copies>(id.id24);
        VkDevice& dev = resourceTableAllocator.Get<ResourceTable_Device>(id.id24);

        // because we store the write-infos in the other list, and the VkWriteDescriptorSet wants a pointer to the structure
        // we need to re-assign the pointers, but thankfully they have values from before
        IndexT i;
        for (i = 0; i < writeList.Size(); i++)
        {
            if (writeList[i].pBufferInfo != nullptr) writeList[i].pBufferInfo = &infoList[i].buf;
            if (writeList[i].pImageInfo != nullptr) writeList[i].pImageInfo = &infoList[i].img;
            if (writeList[i].pTexelBufferView != nullptr) writeList[i].pTexelBufferView = &infoList[i].tex;
        }
        if (writeList.Size() > 0 || copies.Size() > 0)
        {
            vkUpdateDescriptorSets(dev, writeList.Size(), writeList.Begin(), copies.Size(), copies.Begin());
            writeList.Free();
            infoList.Free();
            copies.Free();
        }
    }
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
    Util::Array<VkDescriptorSetLayoutBinding> bindings;

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

            // accumulate textures for same bind point
            IndexT countIndex = textureCount.FindIndex(binding.binding);
            if (countIndex == InvalidIndex)
                textureCount.Add(binding.binding, tex.num);
            else
            {
                uint32_t& count = textureCount.ValueAtIndex(binding.binding, countIndex);
                count = Math::max(count, (uint32_t)tex.num);
            }
        }
        else
        {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.pImmutableSamplers = &SamplerGetVk(tex.immutableSampler);
            if (!immutable.Contains(tex.slot))
                immutable.Add(tex.slot, true);

            // accumulate textures for same bind point
            IndexT countIndex = sampledTextureCount.FindIndex(binding.binding);
            if (countIndex == InvalidIndex)
                sampledTextureCount.Add(binding.binding, tex.num);
            else
            {
                uint32_t& count = sampledTextureCount.ValueAtIndex(binding.binding, countIndex);
                count = Math::max(count, (uint32_t)tex.num);
            }
        }
        binding.stageFlags = VkTypes::AsVkShaderVisibility(tex.visibility);
        bindings.Append(binding);
    }

    // update pool sizes
    auto textureCountIt = textureCount.Begin();
    while (textureCountIt != textureCount.End())
    {
        sampledImageSize.descriptorCount += *textureCountIt.val;
        textureCountIt++;
    }

    auto sampledTextureCountIt = sampledTextureCount.Begin();
    while (sampledTextureCountIt != sampledTextureCount.End())
    {
        combinedImageSize.descriptorCount += *sampledTextureCountIt.val;
        sampledTextureCountIt++;
    }

    // add to list of sizes
    if (sampledImageSize.descriptorCount > 0)
        poolSizes.Append(sampledImageSize);
    if (combinedImageSize.descriptorCount > 0)
        poolSizes.Append(combinedImageSize);

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
        bindings.Append(binding);

        // accumulate textures for same bind point
        IndexT countIndex = imageCount.FindIndex(binding.binding);
        if (countIndex == InvalidIndex)
            imageCount.Add(binding.binding, tex.num);
        else
        {
            uint32_t& count = imageCount.ValueAtIndex(binding.binding, countIndex);
            count = Math::max(count, (uint32_t)tex.num);
        }
    }

    auto imageCountIt = imageCount.Begin();
    while (imageCountIt != imageCount.End())
    {
        rwImageSize.descriptorCount += *imageCountIt.val;
        imageCountIt++;
    }

    // add to list of sizes
    if (rwImageSize.descriptorCount > 0)
        poolSizes.Append(rwImageSize);


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
        bindings.Append(binding);

        if (buf.dynamicOffset)
        {
            // accumulate textures for same bind point
            IndexT countIndex = dynamicUniformCount.FindIndex(binding.binding);
            if (countIndex == InvalidIndex)
                dynamicUniformCount.Add(binding.binding, buf.num);
            else
            {
                uint32_t& count = dynamicUniformCount.ValueAtIndex(binding.binding, countIndex);
                count = Math::max(count, (uint32_t)buf.num);
            }
        }
        else
        {
            // accumulate textures for same bind point
            IndexT countIndex = uniformCount.FindIndex(binding.binding);
            if (countIndex == InvalidIndex)
                uniformCount.Add(binding.binding, buf.num);
            else
            {
                uint32_t& count = uniformCount.ValueAtIndex(binding.binding, countIndex);
                count = Math::max(count, (uint32_t)buf.num);
            }
        }
    }

    // update pool sizes
    auto dynamicUniformCountIt = dynamicUniformCount.Begin();
    while (dynamicUniformCountIt != dynamicUniformCount.End())
    {
        cbDynamicSize.descriptorCount += *dynamicUniformCountIt.val;
        dynamicUniformCountIt++;
    }

    auto uniformCountIt = uniformCount.Begin();
    while (uniformCountIt != uniformCount.End())
    {
        cbSize.descriptorCount += *uniformCountIt.val;
        uniformCountIt++;
    }

    // add to list of sizes
    if (cbDynamicSize.descriptorCount > 0)
        poolSizes.Append(cbDynamicSize);
    if (cbSize.descriptorCount > 0)
        poolSizes.Append(cbSize);


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
        bindings.Append(binding);

        if (buf.dynamicOffset)
        {
            // accumulate textures for same bind point
            IndexT countIndex = dynamicBufferCount.FindIndex(binding.binding);
            if (countIndex == InvalidIndex)
                dynamicBufferCount.Add(binding.binding, buf.num);
            else
            {
                uint32_t& count = dynamicBufferCount.ValueAtIndex(binding.binding, countIndex);
                count = Math::max(count, (uint32_t)buf.num);
            }
        }
        else
        {
            // accumulate textures for same bind point
            IndexT countIndex = bufferCount.FindIndex(binding.binding);
            if (countIndex == InvalidIndex)
                bufferCount.Add(binding.binding, buf.num);
            else
            {
                uint32_t& count = bufferCount.ValueAtIndex(binding.binding, countIndex);
                count = Math::max(count, (uint32_t)buf.num);
            }
        }
    }

        // update pool sizes
    auto dynamicBufferCountIt = dynamicBufferCount.Begin();
    while (dynamicBufferCountIt != dynamicBufferCount.End())
    {
        rwDynamicBufferSize.descriptorCount += *dynamicBufferCountIt.val;
        dynamicBufferCountIt++;
    }

    auto bufferCountIt = bufferCount.Begin();
    while (bufferCountIt != bufferCount.End())
    {
        rwBufferSize.descriptorCount += *bufferCountIt.val;
        bufferCountIt++;
    }

    // add to list of sizes
    if (rwDynamicBufferSize.descriptorCount > 0)
        poolSizes.Append(rwDynamicBufferSize);
    if (rwBufferSize.descriptorCount > 0)
        poolSizes.Append(rwBufferSize);


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
        bindings.Append(binding);

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
        bindings.Append(binding);
        inputAttachmentSize.descriptorCount += tex.num;
    }

    if (inputAttachmentSize.descriptorCount > 0)
        poolSizes.Append(inputAttachmentSize);

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
        VkDescriptorSetLayoutCreateInfo dslInfo =
        {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            &bindingFlags,
            0,                                                              // USE vkCmdPushDescriptorSetKHR IN THE FUTURE!
            (uint32_t)bindings.Size(),
            bindings.Begin()
        };
        VkResult res = vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &layout);
        n_assert(res == VK_SUCCESS);
    }

    ResourceTableLayoutId ret;
    ret.id24 = id;
    ret.id8 = ResourceTableLayoutIdType;

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourceTableLayout(const ResourceTableLayoutId& id)
{
    VkDevice& dev = resourceTableLayoutAllocator.Get<ResourceTableLayout_Device>(id.id24);
    VkDescriptorSetLayout& layout = resourceTableLayoutAllocator.Get<ResourceTableLayout_SetLayout>(id.id24);
    vkDestroyDescriptorSetLayout(dev, layout, nullptr);

    // destroy all pools
    Util::Array<VkDescriptorPool>& pools = resourceTableLayoutAllocator.Get<ResourceTableLayout_DescriptorPools>(id.id24);
    for (IndexT i = 0; i < pools.Size(); i++)
        vkDestroyDescriptorPool(dev, pools[i], nullptr);

    pools.Clear();

    resourceTableLayoutAllocator.Dealloc(id.id24);
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
            layouts.Append(emptySetLayout);
        }
        layouts.Append(resourceTableLayoutAllocator.Get<ResourceTableLayout_SetLayout>(info.tables[i].id24));
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

    ResourcePipelineId ret;
    ret.id24 = id;
    ret.id8 = ResourcePipelineIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourcePipeline(const ResourcePipelineId& id)
{
    VkDevice& dev = resourcePipelineAllocator.Get<0>(id.id24);
    VkPipelineLayout& layout = resourcePipelineAllocator.Get<1>(id.id24);
    vkDestroyPipelineLayout(dev, layout, nullptr);

    resourcePipelineAllocator.Dealloc(id.id24);
}

} // namespace CoreGraphics
