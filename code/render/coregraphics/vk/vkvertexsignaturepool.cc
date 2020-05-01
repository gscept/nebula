//------------------------------------------------------------------------------
// vkvertexsignaturepool.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkvertexsignaturepool.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/shader.h"
#include "vktypes.h"
#include "shader.h"
#include "coregraphics/shaderpool.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkVertexSignaturePool, 'VVSP', Resources::ResourceMemoryPool);
//------------------------------------------------------------------------------
/**
*/
VkVertexSignaturePool::VkVertexSignaturePool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkVertexSignaturePool::~VkVertexSignaturePool()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkVertexSignaturePool::GetVertexLayoutSize(const CoreGraphics::VertexLayoutId id)
{
	return this->Get<3>(id.resourceId).vertexByteSize;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<CoreGraphics::VertexComponent>&
VkVertexSignaturePool::GetVertexComponents(const CoreGraphics::VertexLayoutId id)
{
	return this->Get<3>(id.resourceId).comps;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkVertexSignaturePool::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
	const CoreGraphics::VertexLayoutInfo* vertexLayoutInfo = reinterpret_cast<const CoreGraphics::VertexLayoutInfo*>(info);
	Util::HashTable<uint64_t, DerivativeLayout>& hashTable = this->Get<0>(id.resourceId);
	VkPipelineVertexInputStateCreateInfo& vertexInfo = this->Get<1>(id.resourceId);
	BindInfo& bindInfo = this->Get<2>(id.resourceId);
	this->Get<3>(id.resourceId) = *vertexLayoutInfo;

	// create binds
	bindInfo.binds.Resize(CoreGraphics::MaxNumVertexStreams);
	bindInfo.attrs.Resize(vertexLayoutInfo->comps.Size());

	SizeT strides[CoreGraphics::MaxNumVertexStreams] = { 0 };

	uint32_t numUsedStreams = 0;
	IndexT curOffset[CoreGraphics::MaxNumVertexStreams];
	bool usedStreams[CoreGraphics::MaxNumVertexStreams];
	Memory::Fill(curOffset, CoreGraphics::MaxNumVertexStreams * sizeof(IndexT), 0);
	Memory::Fill(usedStreams, CoreGraphics::MaxNumVertexStreams * sizeof(bool), 0);

	IndexT compIndex;
	for (compIndex = 0; compIndex < vertexLayoutInfo->comps.Size(); compIndex++)
	{
		const CoreGraphics::VertexComponent& component = vertexLayoutInfo->comps[compIndex];
		VkVertexInputAttributeDescription* attr = &bindInfo.attrs[compIndex];

		attr->location = component.GetSemanticName();
		attr->binding = component.GetStreamIndex();
		attr->format = VkTypes::AsVkVertexType(component.GetFormat());
		attr->offset = curOffset[component.GetStreamIndex()];

		if (usedStreams[attr->binding])
			bindInfo.binds[attr->binding].stride += component.GetByteSize();
		else
		{
			bindInfo.binds[attr->binding].stride = component.GetByteSize();
			usedStreams[attr->binding] = true;
			numUsedStreams++;
		}

		bindInfo.binds[attr->binding].binding = component.GetStreamIndex();
		bindInfo.binds[attr->binding].inputRate = component.GetStrideType() == CoreGraphics::VertexComponent::PerVertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
		curOffset[component.GetStreamIndex()] += component.GetByteSize();
	}

	vertexInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		NULL,
		0,
		numUsedStreams,
		bindInfo.binds.Begin(),
		(uint32_t)bindInfo.attrs.Size(),
		bindInfo.attrs.Begin()
	};

	// pepperidge farms remember
	this->states[id.poolId] = Resources::Resource::Loaded;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexSignaturePool::Unload(const Resources::ResourceId id)
{
	// clear the table as it may not be reused by the next layout
	Util::HashTable<uint64_t, DerivativeLayout>& hashTable = this->Get<0>(id.resourceId);
	hashTable.Clear();

	// also clear bind info
	BindInfo& bindInfo = this->Get<2>(id.resourceId);
	bindInfo.binds.Clear();
	bindInfo.attrs.Clear();
}

//------------------------------------------------------------------------------
/**
	The default constructor for HashTable takes 128 entries, and it should be sufficient...
	However, if we ever run into any hash collision issues, extend the size on load.
*/
VkPipelineVertexInputStateCreateInfo*
VkVertexSignaturePool::GetDerivativeLayout(const CoreGraphics::VertexLayoutId layout, const CoreGraphics::ShaderProgramId shader)
{
	Util::HashTable<uint64_t, DerivativeLayout>& hashTable = this->Get<0>(layout.resourceId);
	AnyFX::VkProgram* program = CoreGraphics::shaderPool->GetProgram(shader);
	const BindInfo& bindInfo = this->Get<2>(layout.resourceId);
	const VkPipelineVertexInputStateCreateInfo& baseInfo = this->Get<1>(layout.resourceId);
	const Ids::Id64 shaderHash = shader.HashCode64();
	IndexT i = hashTable.FindIndex(shaderHash);
	if (i != InvalidIndex)
	{
		return &hashTable.ValueAtIndex(shaderHash, i).info;
	}
	else
	{
		IndexT index = hashTable.Add(shaderHash, {});
		DerivativeLayout& layout = hashTable.ValueAtIndex(shaderHash, index);
		AnyFX::VkProgram* program = CoreGraphics::shaderPool->GetProgram(shader);
		layout.info = baseInfo;

		uint32_t i;
		IndexT j;
		for (i = 0; i < program->vsInputSlots.size(); i++)
		{
			uint32_t slot = program->vsInputSlots[i];
			for (j = 0; j < bindInfo.attrs.Size(); j++)
			{
				VkVertexInputAttributeDescription attr = bindInfo.attrs[j];
				if (attr.location == slot)
				{
					layout.attrs.Append(attr);
					break;
				}
			}
		}

		layout.info.vertexAttributeDescriptionCount = layout.attrs.Size();
		layout.info.pVertexAttributeDescriptions = layout.attrs.Begin();
		return &layout.info;
	}
}

} // namespace Vertex
