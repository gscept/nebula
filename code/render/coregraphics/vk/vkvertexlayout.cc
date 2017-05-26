//------------------------------------------------------------------------------
// vkvertexlayout.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkvertexlayout.h"
#include "vkrenderdevice.h"
#include "vktypes.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkVertexLayout, 'VKVL', Base::VertexLayoutBase);
//------------------------------------------------------------------------------
/**
*/
VkVertexLayout::VkVertexLayout() :
	derivatives(128)
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
VkVertexLayout::Setup(const Util::Array<CoreGraphics::VertexComponent>& c)
{
	// call parent class
	Base::VertexLayoutBase::Setup(c);

	// create binds
	this->binds.Resize(MaxNumVertexStreams);
	this->attrs.Resize(this->components.Size());

	SizeT strides[MaxNumVertexStreams] = { 0 };

	uint32_t numUsedStreams = 0;
	IndexT streamIndex;
	for (streamIndex = 0; streamIndex < MaxNumVertexStreams; streamIndex++)
	{
		if (this->usedStreams[streamIndex])
		{
			this->binds[numUsedStreams].binding = numUsedStreams;
			this->binds[numUsedStreams].inputRate = numUsedStreams > 0 ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
			this->binds[numUsedStreams].stride = 0;
			numUsedStreams++;
		}		
	}
	IndexT curOffset[MaxNumVertexStreams] = { 0 };

	IndexT compIndex;
	for (compIndex = 0; compIndex < this->components.Size(); compIndex++)
	{
		const CoreGraphics::VertexComponent& component = this->components[compIndex];
		VkVertexInputAttributeDescription* attr = &this->attrs[compIndex];

		attr->location = component.GetSemanticName();
		attr->binding = component.GetStreamIndex();
		attr->format = VkTypes::AsVkVertexType(component.GetFormat());
		attr->offset = curOffset[component.GetStreamIndex()];
		curOffset[component.GetStreamIndex()] += component.GetByteSize();
		this->binds[attr->binding].stride += component.GetByteSize();
	}

	this->vertexInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		NULL,
		0,
		numUsedStreams,
		this->binds.Begin(),
		(uint32_t)this->attrs.Size(),
		this->attrs.Begin()
	};

	// finish up the info struct
	this->info.pVertexInputState = &this->vertexInfo;
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
VkVertexLayout::CreateDerivative(const Ptr<VkShaderProgram>& program)
{
	const AnyFX::VkProgram* prog = program->GetVkProgram();

	DerivativeLayout* derivative = n_new(DerivativeLayout);
	derivative->info = this->vertexInfo;

	uint32_t i;
	IndexT j;
	for (i = 0; i < prog->vsInputSlots.size(); i++)
	{
		uint32_t slot = prog->vsInputSlots[i];
		for (j = 0; j < this->attrs.Size(); j++)
		{
			VkVertexInputAttributeDescription attr = this->attrs[j];
			if (attr.location == slot)
			{
				derivative->attrs.Append(attr);
				break;
			}
		}
		
	}
	derivative->info.vertexAttributeDescriptionCount = derivative->attrs.Size();
	derivative->info.pVertexAttributeDescriptions = derivative->attrs.Begin();
	this->derivatives.Add(program, derivative);
	return &derivative->info;
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineVertexInputStateCreateInfo*
VkVertexLayout::GetDerivative(const Ptr<VkShaderProgram>& program)
{
	if (this->derivatives.Contains(program)) return &this->derivatives[program]->info;
	else
	{
		return this->CreateDerivative(program);
	}
}

} // namespace Vulkan
