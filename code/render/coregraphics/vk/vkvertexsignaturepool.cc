//------------------------------------------------------------------------------
// vkvertexsignaturepool.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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
void
VkVertexSignaturePool::VertexLayoutBind(const CoreGraphics::VertexLayoutId id)
{
	n_error("I am not complete!\n");
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkVertexSignaturePool::VertexLayoutGetSize(const CoreGraphics::VertexLayoutId id)
{
	return this->Get<3>(id.id24).vertexByteSize;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkVertexSignaturePool::LoadFromMemory(const Ids::Id24 id, void* info)
{
	CoreGraphics::VertexLayoutInfo* vertexLayoutInfo = (CoreGraphics::VertexLayoutInfo*)info;
	Util::HashTable<DerivativeId, DerivativeLayout>& hashTable = this->Get<0>(id);
	VkPipelineVertexInputStateCreateInfo& vertexInfo = this->Get<1>(id);
	BindInfo& bindInfo = this->Get<2>(id);

	// create binds
	bindInfo.binds.Resize(CoreGraphics::MaxNumVertexStreams);
	bindInfo.attrs.Resize(vertexLayoutInfo->comps.Size());

	SizeT strides[CoreGraphics::MaxNumVertexStreams] = { 0 };

	uint32_t numUsedStreams = 0;
	IndexT streamIndex;
	for (streamIndex = 0; streamIndex < CoreGraphics::MaxNumVertexStreams; streamIndex++)
	{
		if (vertexLayoutInfo->usedStreams[streamIndex])
		{
			bindInfo.binds[numUsedStreams].binding = numUsedStreams;
			bindInfo.binds[numUsedStreams].inputRate = numUsedStreams > 0 ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
			bindInfo.binds[numUsedStreams].stride = 0;
			numUsedStreams++;
		}
	}
	IndexT curOffset[CoreGraphics::MaxNumVertexStreams] = { 0 };

	IndexT compIndex;
	for (compIndex = 0; compIndex < vertexLayoutInfo->comps.Size(); compIndex++)
	{
		const CoreGraphics::VertexComponent& component = vertexLayoutInfo->comps[compIndex];
		VkVertexInputAttributeDescription* attr = &bindInfo.attrs[compIndex];

		attr->location = component.GetSemanticName();
		attr->binding = component.GetStreamIndex();
		attr->format = VkTypes::AsVkVertexType(component.GetFormat());
		attr->offset = curOffset[component.GetStreamIndex()];
		curOffset[component.GetStreamIndex()] += component.GetByteSize();
		bindInfo.binds[attr->binding].stride += component.GetByteSize();
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
	this->states[id] = Resources::Resource::Loaded;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexSignaturePool::Unload(const Ids::Id24 id)
{
	// clear the table as it may not be reused by the next layout
	Util::HashTable<DerivativeId, DerivativeLayout>& hashTable = this->Get<0>(id);
	hashTable.Clear();

	// also clear bind info
	BindInfo& bindInfo = this->Get<2>(id);
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
	Util::HashTable<DerivativeId, DerivativeLayout>& hashTable = this->Get<0>(layout.id24);
	const BindInfo& bindInfo = this->Get<2>(layout.id24);
	const VkPipelineVertexInputStateCreateInfo& baseInfo = this->Get<1>(layout.id24);
	const Ids::Id64 shaderHash = shader.HashCode64();
	if (hashTable.Contains(shaderHash))
	{
		return &hashTable[shaderHash].info;
	}
	else
	{
		DerivativeLayout layout;
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
		hashTable.Add(shaderHash, layout);
		return &hashTable[shaderHash].info;
	}
}

} // namespace Vertex
