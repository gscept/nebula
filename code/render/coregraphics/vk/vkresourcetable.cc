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
#include "vkconstantbuffer.h"
namespace Vulkan
{

VkResourceTableAllocator resourceTableAllocator;
VkResourceTableLayoutAllocator resourceTableLayoutAllocator;
VkResourcePipelineAllocator resourcePipelineAllocator;
VkDescriptorSetLayout emptySetLayout;

static bool ResourceTableBlocked = false;

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorSet&
ResourceTableGetVkDescriptorSet(const CoreGraphics::ResourceTableId& id)
{
	return resourceTableAllocator.Get<1>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorSetLayout&
ResourceTableGetVkLayout(const CoreGraphics::ResourceTableId& id)
{
	return ResourceTableLayoutGetVk(resourceTableAllocator.Get<3>(id.id24));
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
	return resourceTableLayoutAllocator.Get<ResourceTableLayoutSetLayout>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorPool& 
ResourceTableLayoutGetPool(const CoreGraphics::ResourceTableLayoutId& id)
{
	return resourceTableLayoutAllocator.Get<ResourceTableLayoutCurrentPool>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkDescriptorPool& 
ResourceTableLayoutNewPool(const CoreGraphics::ResourceTableLayoutId& id)
{
	// orphan old pool
	Util::Array<VkDescriptorPoolSize>& poolSizes = resourceTableLayoutAllocator.Get<ResourceTableLayoutPoolSizes>(id.id24);
	const VkDevice& dev = resourceTableLayoutAllocator.Get<ResourceTableLayoutDevice>(id.id24);
	uint32_t& grow = resourceTableLayoutAllocator.Get<ResourceTableLayoutPoolGrow>(id.id24);

	// create new pool
	VkDescriptorPoolCreateInfo poolInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		grow,
		(uint32_t)poolSizes.Size(),
		poolSizes.Size() > 0 ? poolSizes.Begin() : nullptr
	};

	// grow, but also clamp to 65535 so as to not grow too much
	grow = Math::n_min(grow << 2, 65535u);

	VkDescriptorPool pool;
	VkResult res = vkCreateDescriptorPool(dev, &poolInfo, nullptr, &pool);
	n_assert(res == VK_SUCCESS);

	// add to list of pools, and set new pointer to the new pool
	resourceTableLayoutAllocator.Get<ResourceTableLayoutDescriptorPools>(id.id24).Append(pool);
	resourceTableLayoutAllocator.Get<ResourceTableLayoutCurrentPool>(id.id24) = pool;
	return resourceTableLayoutAllocator.Get<ResourceTableLayoutDescriptorPools>(id.id24).Back();
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

//------------------------------------------------------------------------------
/**
*/
ResourceTableId
CreateResourceTable(const ResourceTableCreateInfo& info)
{
	Ids::Id32 id = resourceTableAllocator.Alloc();

	VkDevice& dev = resourceTableAllocator.Get<0>(id);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id);
	VkDescriptorPool& pool = resourceTableAllocator.Get<2>(id);
	CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<3>(id);

	dev = Vulkan::GetCurrentDevice();
	layout = info.layout;
	pool = ResourceTableLayoutGetPool(layout);

	VkDescriptorSetAllocateInfo dsetAlloc =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		pool,
		1,
		&ResourceTableLayoutGetVk(layout)
	};
	VkResult res = vkAllocateDescriptorSets(dev, &dsetAlloc, &set);

	// if we are full, request new pool
	if (res == VK_ERROR_OUT_OF_POOL_MEMORY)
	{
		pool = ResourceTableLayoutNewPool(layout);
		dsetAlloc.descriptorPool = pool;
		VkResult res = vkAllocateDescriptorSets(dev, &dsetAlloc, &set);
		n_assert(res == VK_SUCCESS);
	}

	ResourceTableId ret;
	ret.id24 = id;
	ret.id8 = ResourceTableIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourceTable(const ResourceTableId& id)
{
    n_assert(id != ResourceTableId::Invalid());
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	VkDescriptorPool& pool = resourceTableAllocator.Get<2>(id.id24);
	vkFreeDescriptorSets(dev, pool, 1, &set);

	resourceTableAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetTexture(const ResourceTableId& id, const ResourceTableTexture& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

	n_assert(tex.slot != InvalidIndex);

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;

	const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<3>(id.id24);
	const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayoutImmutableSamplerFlags>(layout.id24);

	VkDescriptorImageInfo img;
	if (immutable[tex.slot])
	{
		n_assert(tex.sampler == SamplerId::Invalid());
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		img.sampler = VK_NULL_HANDLE;
	}
	else
	{
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		img.sampler = tex.sampler == SamplerId::Invalid() ? VK_NULL_HANDLE : SamplerGetVk(tex.sampler);
	}

	write.descriptorCount = 1;
	write.dstArrayElement = tex.index;
	write.dstBinding = tex.slot;
	write.dstSet = set;
	img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (tex.tex == TextureId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else if (tex.isStencil)
		img.imageView = TextureGetVkStencilImageView(tex.tex);
	else
		img.imageView = TextureGetVkImageView(tex.tex);

	WriteInfo inf;
	inf.img = img;
	infoList.Append(inf);

	write.pImageInfo = &img;			// this is just provisionary, it will go out of scope immediately, but it wont be null!
	write.pTexelBufferView = nullptr;
	write.pBufferInfo = nullptr;

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetTexture(const ResourceTableId& id, const ResourceTableTextureView& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

	n_assert(tex.slot != InvalidIndex);

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;

	const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<3>(id.id24);
	const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayoutImmutableSamplerFlags>(layout.id24);

	VkDescriptorImageInfo img;
	if (immutable[tex.slot])
	{
		n_assert(tex.sampler == SamplerId::Invalid());
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		img.sampler = VK_NULL_HANDLE;
	}
	else
	{
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		img.sampler = tex.sampler == SamplerId::Invalid() ? VK_NULL_HANDLE : SamplerGetVk(tex.sampler);
	}

	write.descriptorCount = 1;
	write.dstArrayElement = tex.index;
	write.dstBinding = tex.slot;
	write.dstSet = set;
	img.imageLayout = tex.isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (tex.tex == TextureViewId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = TextureViewGetVk(tex.tex);

	WriteInfo inf;
	inf.img = img;
	infoList.Append(inf);

	write.pImageInfo = &img;			// this is just provisionary, it will go out of scope immediately, but it wont be null!
	write.pTexelBufferView = nullptr;
	write.pBufferInfo = nullptr;

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetInputAttachment(const ResourceTableId& id, const ResourceTableInputAttachment& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (tex.tex == TextureViewId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = TextureViewGetVk(tex.tex);

	WriteInfo inf;
	inf.img = img;
	infoList.Append(inf);

	write.pImageInfo = &img;			// this is just provisionary, it will go out of scope immediately, but it wont be null!
	write.pTexelBufferView = nullptr;
	write.pBufferInfo = nullptr;

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetRWTexture(const ResourceTableId& id, const ResourceTableTexture& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (tex.tex == TextureId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = TextureGetVkImageView(tex.tex);

	WriteInfo inf;
	inf.img = img;
	infoList.Append(inf);

	write.pImageInfo = &img;			// this is just provisionary, it will go out of scope immediately, but it wont be null!
	write.pTexelBufferView = nullptr;
	write.pBufferInfo = nullptr;

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetRWTexture(const ResourceTableId& id, const ResourceTableTextureView& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (tex.tex == TextureViewId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = TextureViewGetVk(tex.tex);

	WriteInfo inf;
	inf.img = img;
	infoList.Append(inf);

	write.pImageInfo = &img;			// this is just provisionary, it will go out of scope immediately, but it wont be null!
	write.pTexelBufferView = nullptr;
	write.pBufferInfo = nullptr;

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetConstantBuffer(const ResourceTableId& id, const ResourceTableBuffer& buf)
{
	n_assert(!buf.texelBuffer);
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (buf.buf == BufferId::Invalid())
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
	write.pBufferInfo = &buff;			// this is just provisionary, it will go out of scope immediately, but it wont be null!

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceTableSetRWBuffer(const ResourceTableId& id, const ResourceTableBuffer& buf)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (buf.buf == BufferId::Invalid())
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
	write.pBufferInfo = &buff;			// this is just provisionary, it will go out of scope immediately, but it wont be null!

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetConstantBuffer(const ResourceTableId& id, const ResourceTableConstantBuffer& buf)
{
	n_assert(!buf.texelBuffer);
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (buf.buf == ConstantBufferId::Invalid())
		buff.buffer = VK_NULL_HANDLE;
	else
		buff.buffer = ConstantBufferGetVk(buf.buf);
	buff.offset = buf.offset;
	buff.range = buf.size == NEBULA_WHOLE_BUFFER_SIZE ? VK_WHOLE_SIZE : buf.size;

	WriteInfo inf;
	inf.buf = buff;
	infoList.Append(inf);

	write.pImageInfo = nullptr;
	write.pTexelBufferView = nullptr;
	write.pBufferInfo = &buff;			// this is just provisionary, it will go out of scope immediately, but it wont be null!

	writeList.Append(write);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableSetSampler(const ResourceTableId& id, const ResourceTableSampler& samp)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (samp.samp == SamplerId::Invalid())
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
	ResourceTableBlocked = b;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceTableCommitChanges(const ResourceTableId& id)
{
	n_assert2(!ResourceTableBlocked, "Resource table updates are blocked! Please move your resource table update code to UpdateViewDepdendentResources or UpdateResources");
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);

	// because we store the write-infos in the other list, and the VkWriteDescriptorSet wants a pointer to the structure
	// we need to re-assign the pointers, but thankfully they have values from before
	IndexT i;
	for (i = 0; i < writeList.Size(); i++)
	{
		if (writeList[i].pBufferInfo != nullptr) writeList[i].pBufferInfo = &infoList[i].buf;
		if (writeList[i].pImageInfo != nullptr) writeList[i].pImageInfo = &infoList[i].img;
		if (writeList[i].pTexelBufferView != nullptr) writeList[i].pTexelBufferView = &infoList[i].tex;
	}
	if (i != 0) 
	{
		vkUpdateDescriptorSets(dev, writeList.Size(), writeList.Begin(), 0, nullptr);
		writeList.Free();
		infoList.Free();
	}
}

//------------------------------------------------------------------------------
/**
*/
ResourceTableLayoutId
CreateResourceTableLayout(const ResourceTableLayoutCreateInfo& info)
{
	Ids::Id32 id = resourceTableLayoutAllocator.Alloc();

	VkDevice& dev = resourceTableLayoutAllocator.Get<ResourceTableLayoutDevice>(id);
	VkDescriptorSetLayout& layout = resourceTableLayoutAllocator.Get<ResourceTableLayoutSetLayout>(id);
	Util::Array<Util::Pair<CoreGraphics::SamplerId, uint32_t>>& samplers = resourceTableLayoutAllocator.Get<ResourceTableLayoutSamplers>(id);
	Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<ResourceTableLayoutImmutableSamplerFlags>(id);
	Util::Array<VkDescriptorPoolSize>& poolSizes = resourceTableLayoutAllocator.Get<ResourceTableLayoutPoolSizes>(id);

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

	// setup textures, or texture-sampler pairs
	for (IndexT i = 0; i < info.textures.Size(); i++)
	{
		const ResourceTableLayoutTexture& tex = info.textures[i];
		n_assert(tex.num >= 0);
		VkDescriptorSetLayoutBinding binding;
		binding.binding = tex.slot;
		binding.descriptorCount = tex.num;
		if (tex.immutableSampler == SamplerId::Invalid())
		{
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			binding.pImmutableSamplers = nullptr;
			immutable.Add(tex.slot, false);
			sampledImageSize.descriptorCount += tex.num;
		}
		else
		{
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.pImmutableSamplers = &SamplerGetVk(tex.immutableSampler);
			immutable.Add(tex.slot, true);
			combinedImageSize.descriptorCount += tex.num;
		}
		binding.stageFlags = VkTypes::AsVkShaderVisibility(tex.visibility);
		bindings.Append(binding);
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
		n_assert(tex.immutableSampler == SamplerId::Invalid());
		VkDescriptorSetLayoutBinding binding;
		binding.binding = tex.slot;
		binding.descriptorCount = tex.num;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VkTypes::AsVkShaderVisibility(tex.visibility);
		bindings.Append(binding);
		rwImageSize.descriptorCount += tex.num;
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
		buf.dynamicOffset ? cbDynamicSize.descriptorCount += buf.num : cbSize.descriptorCount += buf.num;
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
		buf.dynamicOffset ? rwDynamicBufferSize.descriptorCount += buf.num : rwBufferSize.descriptorCount += buf.num;
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
		VkDescriptorSetLayoutCreateInfo dslInfo =
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,																// USE vkCmdPushDescriptorSetKHR IN THE FUTURE!
			(uint32_t)bindings.Size(),
			bindings.Begin()
		};
		VkResult res = vkCreateDescriptorSetLayout(dev, &dslInfo, nullptr, &layout);
		n_assert(res == VK_SUCCESS);
	}

	ResourceTableLayoutId ret;
	ret.id24 = id;
	ret.id8 = ResourceTableLayoutIdType;

	// set initial grow and create an initial resource pool
	resourceTableLayoutAllocator.Get<ResourceTableLayoutPoolGrow>(id) = info.descriptorPoolInitialGrow;
	ResourceTableLayoutNewPool(ret);

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourceTableLayout(const ResourceTableLayoutId& id)
{
	VkDevice& dev = resourceTableLayoutAllocator.Get<ResourceTableLayoutDevice>(id.id24);
	VkDescriptorSetLayout& layout = resourceTableLayoutAllocator.Get<ResourceTableLayoutSetLayout>(id.id24);
	vkDestroyDescriptorSetLayout(dev, layout, nullptr);

	// destroy all pools
	Util::Array<VkDescriptorPool>& pools = resourceTableLayoutAllocator.Get<ResourceTableLayoutDescriptorPools>(id.id24);
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
		layouts.Append(resourceTableLayoutAllocator.Get<ResourceTableLayoutSetLayout>(info.tables[i].id24));
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
