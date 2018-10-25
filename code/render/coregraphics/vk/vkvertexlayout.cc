//------------------------------------------------------------------------------
// vkvertexlayout.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkvertexlayout.h"
#include "vkrenderdevice.h"
#include "vktypes.h"

namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
VkVertexLayout::VkVertexLayout()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkVertexLayout::~VkVertexLayout()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexLayout::Setup(BindInfo& info, VertexLayoutBaseInfo& baseInfo, VkPipelineVertexInputStateCreateInfo& vertexInfo, const Util::Array<CoreGraphics::VertexComponent>& c)
{
	// call parent class
	Base::VertexLayoutBase::Setup(c, baseInfo);

	// create binds
	info.binds.Resize(MaxNumVertexStreams);
	info.attrs.Resize(c.Size());

	SizeT strides[MaxNumVertexStreams] = { 0 };

	uint32_t numUsedStreams = 0;
	IndexT streamIndex;
	for (streamIndex = 0; streamIndex < MaxNumVertexStreams; streamIndex++)
	{
		if (baseInfo.usedStreams[streamIndex])
		{
			info.binds[numUsedStreams].binding = numUsedStreams;
			info.binds[numUsedStreams].inputRate = numUsedStreams > 0 ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
			info.binds[numUsedStreams].stride = 0;
			numUsedStreams++;
		}		
	}
	IndexT curOffset[MaxNumVertexStreams] = { 0 };

	IndexT compIndex;
	for (compIndex = 0; compIndex < baseInfo.components.Size(); compIndex++)
	{
		const CoreGraphics::VertexComponent& component = baseInfo.components[compIndex];
		VkVertexInputAttributeDescription* attr = &info.attrs[compIndex];

		attr->location = component.GetSemanticName();
		attr->binding = component.GetStreamIndex();
		attr->format = VkTypes::AsVkVertexType(component.GetFormat());
		attr->offset = curOffset[component.GetStreamIndex()];
		curOffset[component.GetStreamIndex()] += component.GetByteSize();
		info.binds[attr->binding].stride += component.GetByteSize();
	}

	vertexInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		NULL,
		0,
		numUsedStreams,
		info.binds.Begin(),
		(uint32_t)info.attrs.Size(),
		info.attrs.Begin()
	};
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexLayout::Discard()
{
	const Util::Array<DerivativeLayout*>& dervs = this->derivatives.ValuesAsArray();
	IndexT i;
	for (i = 0; i <dervs.Size(); i++)
	{
		n_delete(dervs[i]);
	}
	this->derivatives.Clear();
	VertexLayoutBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexLayout::Apply()
{
	//VkRenderDevice::Instance()->SetVertexLayoutPipelineInfo(this->info);
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineVertexInputStateCreateInfo*
VkVertexLayout::CreateDerivative(
	const AnyFX::VkProgram* program, 
	const Ids::Id64 id,
	Util::HashTable<Ids::Id64, VkVertexLayout::DerivativeLayout*>& derivativeHashMap,
	Util::Array<VkVertexLayout::DerivativeLayout>& derivatives,
	VkVertexLayout::BindInfo& bindInfo, 
	VkPipelineVertexInputStateCreateInfo& pipelineInfo
	)
{
	derivatives.Append(DerivativeLayout());
	DerivativeLayout* derivative = derivatives.End();
	derivative->info = pipelineInfo;

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
				derivative->attrs.Append(attr);
				break;
			}
		}
		
	}
	derivative->info.vertexAttributeDescriptionCount = derivative->attrs.Size();
	derivative->info.pVertexAttributeDescriptions = derivative->attrs.Begin();
	derivativeHashMap.Add(id, derivative);
	return &derivative->info;
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineVertexInputStateCreateInfo*
VkVertexLayout::GetDerivative(
	const AnyFX::VkProgram* program,
	const Ids::Id64 id,
	Util::HashTable<Ids::Id64, VkVertexLayout::DerivativeLayout*>& derivativeHashMap,
	Util::Array<VkVertexLayout::DerivativeLayout>& derivatives,
	VkVertexLayout::BindInfo& bindInfo,
	VkPipelineVertexInputStateCreateInfo& pipelineInfo
	)
{
	if (derivativeHashMap.Contains(id)) return &derivativeHashMap[id]->info;
	else
	{
		return VkVertexLayout::CreateDerivative(program, id, derivativeHashMap, derivatives, bindInfo, pipelineInfo);
	}
}

} // namespace Vulkan
