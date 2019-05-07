//------------------------------------------------------------------------------
//  vkresourcetable.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkresourcetable.h"
#include "coregraphics/resourcetable.h"
#include "vkgraphicsdevice.h"
#include "vktypes.h"
#include "vksampler.h"
#include "vkshaderrwtexture.h"
#include "vktexture.h"
#include "vkconstantbuffer.h"
#include "vkshaderrwbuffer.h"
#include "vkrendertexture.h"
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
SetupEmptyDescriptorSet()
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
	return resourceTableLayoutAllocator.Get<1>(id.id24);
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
	pool = Vulkan::GetCurrentDescriptorPool();
	layout = info.layout;

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
		Vulkan::RequestDescriptorPool();

		pool = Vulkan::GetCurrentDescriptorPool();
		dsetAlloc.descriptorPool = pool;
		VkResult res = vkAllocateDescriptorSets(dev, &dsetAlloc, &set);
		n_assert(res == VK_SUCCESS);
	}

	ResourceTableId ret;
	ret.id24 = id;
	ret.id8 = ResourceTableIdType;

	// setup samplers
	const Util::Array<std::pair<CoreGraphics::SamplerId, uint32_t>>& samplers = resourceTableLayoutAllocator.Get<2>(info.layout.id24);
	IndexT i;
	for (i = 0; i < samplers.Size(); i++)
	{
		ResourceTableSampler samp;
		samp.samp = std::get<0>(samplers[i]);
		samp.slot = std::get<1>(samplers[i]);

		ResourceTableSetSampler(ret, samp);
	}

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyResourceTable(const ResourceTableId& id)
{
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

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;

	const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<3>(id.id24);
	const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<3>(layout.id24);

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

	if (tex.isDepth)
		img.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	else
		img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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
ResourceTableSetTexture(const ResourceTableId& id, const ResourceTableRenderTexture& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;
		
	write.descriptorCount = 1;
	write.dstArrayElement = tex.index;
	write.dstBinding = tex.slot;
	write.dstSet = set;

	const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<3>(id.id24);
	const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<3>(layout.id24);

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

	if (tex.isDepth)
		img.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	else
		img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (tex.tex == RenderTextureId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = RenderTextureGetVkImageView(tex.tex);

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
ResourceTableSetTexture(const ResourceTableId& id, const ResourceTableShaderRWTexture& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.pNext = nullptr;

	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.descriptorCount = 1;
	write.dstArrayElement = tex.index;
	write.dstBinding = tex.slot;
	write.dstSet = set;

	const CoreGraphics::ResourceTableLayoutId& layout = resourceTableAllocator.Get<3>(id.id24);
	const Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<3>(layout.id24);

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

	img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (tex.tex == ShaderRWTextureId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = ShaderRWTextureGetVkImageView(tex.tex);

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
	img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if (tex.tex == RenderTextureId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = RenderTextureGetVkImageView(tex.tex);

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
ResourceTableSetShaderRWTexture(const ResourceTableId& id, const ResourceTableShaderRWTexture& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (tex.tex == ShaderRWTextureId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = ShaderRWTextureGetVkImageView(tex.tex);

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
ResourceTableSetShaderRWTexture(const ResourceTableId& id, const ResourceTableTexture& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
ResourceTableSetShaderRWTexture(const ResourceTableId& id, const ResourceTableRenderTexture& tex)
{
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (tex.tex == RenderTextureId::Invalid())
		img.imageView = VK_NULL_HANDLE;
	else
		img.imageView = RenderTextureGetVkImageView(tex.tex);

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
ResourceTableSetConstantBuffer(const ResourceTableId& id, const ResourceTableConstantBuffer& buf)
{
	n_assert(!(buf.texelBuffer | buf.texelBuffer));
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	buff.range = buf.size == -1 ? VK_WHOLE_SIZE : buf.size;

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
ResourceTableSetShaderRWBuffer(const ResourceTableId& id, const ResourceTableShaderRWBuffer& buf)
{
	n_assert(!(buf.texelBuffer | buf.texelBuffer));
	VkDevice& dev = resourceTableAllocator.Get<0>(id.id24);
	VkDescriptorSet& set = resourceTableAllocator.Get<1>(id.id24);
	Util::Array<VkWriteDescriptorSet>& writeList = resourceTableAllocator.Get<4>(id.id24);
	Util::Array<WriteInfo>& infoList = resourceTableAllocator.Get<5>(id.id24);

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
	if (buf.buf == ShaderRWBufferId::Invalid())
		buff.buffer = VK_NULL_HANDLE;
	else
		buff.buffer = ShaderRWBufferGetVkBuffer(buf.buf);
	buff.offset = buf.offset;
	buff.range = buf.size == -1 ? VK_WHOLE_SIZE : buf.size;
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
ResourceTableCommitChanges(const ResourceTableId& id)
{
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

	VkDevice& dev = resourceTableLayoutAllocator.Get<0>(id);
	VkDescriptorSetLayout& layout = resourceTableLayoutAllocator.Get<1>(id);
	Util::Array<std::pair<CoreGraphics::SamplerId, uint32_t>>& samplers = resourceTableLayoutAllocator.Get<2>(id);
	Util::HashTable<uint32_t, bool>& immutable = resourceTableLayoutAllocator.Get<3>(id);

	dev = Vulkan::GetCurrentDevice();
	Util::Array<VkDescriptorSetLayoutBinding> bindings;

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
		}
		else
		{
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.pImmutableSamplers = &SamplerGetVk(tex.immutableSampler);
			immutable.Add(tex.slot, true);
		}
		binding.stageFlags = VkTypes::AsVkShaderVisibility(tex.visibility);
		bindings.Append(binding);
	}
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
	}
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
	}
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
	}
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
		samplers.Append(std::make_pair(samp.sampler, samp.slot));
	}
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
	}

	if (bindings.Size() > 0)
	{
		VkDescriptorSetLayoutCreateInfo dslInfo =
		{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,																// USE vkCmdPushDescriptorSetKHR IN THE FUTURE!
			bindings.Size(),
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
	VkDevice& dev = resourceTableLayoutAllocator.Get<0>(id.id24);
	VkDescriptorSetLayout& layout = resourceTableLayoutAllocator.Get<1>(id.id24);
	vkDestroyDescriptorSetLayout(dev, layout, nullptr);

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
		if (info.indices[i] != i)
		{
			IndexT tmp = i;
			while (tmp != info.indices[i])
			{
				layouts.Append(emptySetLayout);
				tmp++;
			}
		}
		layouts.Append(resourceTableLayoutAllocator.Get<1>(info.tables[i].id24));
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
		layouts.Size(),
		layouts.Begin(),
		push.size > 0 ? 1 : 0,
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