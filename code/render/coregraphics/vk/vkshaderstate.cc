//------------------------------------------------------------------------------
// vkshaderstate.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderstate.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shader.h"
#include "vkrenderdevice.h"
#include "coregraphics/shadervariable.h"

using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderState, 'VKSN', Base::ShaderStateBase);
__ImplementClass(Vulkan::VkShaderState::VkDerivativeState, 'VKDE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
VkShaderState::VkShaderState() :
	pushData(NULL),
	pushSize(0),
	setsDirty(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkShaderState::~VkShaderState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::Setup(const Ptr<CoreGraphics::Shader>& origShader, const Util::Array<IndexT>& groups, bool createResourceSet)
{
	n_assert(origShader.isvalid());
	const Ptr<VkShader>& vkShader = origShader.upcast<VkShader>();

	// call parent class
	ShaderStateBase::Setup(origShader);

	// copy effect pointer
	this->effect = vkShader->GetVkEffect();

	// copy sets from shader
	this->sets.Resize(groups.Size());
	this->setBindnings.Resize(groups.Size());
	this->setOffsets.Resize(groups.Size());
	this->setBufferMapping.Resize(groups.Size());
	Util::FixedArray<bool> isActive(groups.Size());
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		bool hasSet = this->shader->sets.Size() > groups[i];
		if (hasSet)
		{
			this->sets[i] = this->shader->sets[groups[i]];
			isActive[i] = true;
		}	
		else
		{
			isActive[i] = false;
			this->sets[i] = VK_NULL_HANDLE;
		}
	}

	// if we want to create our own resource set
	if (createResourceSet)
	{
		for (i = 0; i < groups.Size(); i++)
		{	
			if (isActive[i])
			{
				VkDescriptorSetLayout layout = this->shader->setLayouts[groups[i]];

				// allocate descriptor sets
				VkDescriptorSetAllocateInfo info =
				{
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					NULL,
					VkRenderDevice::descPool,
					1,
					&layout
				};
				VkResult res = vkAllocateDescriptorSets(VkRenderDevice::dev, &info, &this->sets[i]);
				n_assert(res == VK_SUCCESS);
			}			
		}
	}


	// setup variables if we have any layouts
	if (!origShader->setLayouts.IsEmpty())
	{
		this->SetupVariables(groups);
		this->SetupUniformBuffers(groups);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::BeginUpdateSync()
{
	IndexT i;
	for (i = 0; i < this->shader->buffers.Size(); i++)
	{
		this->shader->buffers.ValueAtIndex(i)->BeginUpdateSync();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::EndUpdateSync()
{
	IndexT i;
	for (i = 0; i < this->shader->buffers.Size(); i++)
	{
		this->shader->buffers.ValueAtIndex(i)->EndUpdateSync();
	}
}


//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::Apply()
{
	this->shader->Apply();
	ShaderStateBase::Apply();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::Commit()
{
	if (this->setsDirty)
		this->UpdateDescriptorSets();

	// get render device to apply state
	VkRenderDevice* dev = VkRenderDevice::Instance();

	// now go through and make sure the shader can bind the sets updated
	if (this->shader->activeVariation.isvalid() && dev->currentBindPoint != VkShaderProgram::InvalidType)
	{
		const VkShaderProgram::PipelineType type = dev->currentBindPoint;
		n_assert(type != VkShaderProgram::InvalidType);
		if (type == VkShaderProgram::Graphics)
		{
			// if no variation is being used, bind descriptors for both graphics and compute
			IndexT i;
			for (i = 0; i < this->setBindnings.Size(); i++)
			{
				const DescriptorSetBinding& binding = this->setBindnings[i];
				const Util::Array<uint32_t>& offsets = this->setOffsets[i];
				dev->BindDescriptorsGraphics(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size(), this->applyShared);
			}

			// update push ranges
			if (this->pushSize > 0)
			{
				dev->UpdatePushRanges(VK_SHADER_STAGE_ALL_GRAPHICS, this->pushLayout, 0, this->pushSize, this->pushData);
			}
		}
		else
		{
			// if no variation is being used, bind descriptors for both graphics and compute
			IndexT i;
			for (i = 0; i < this->setBindnings.Size(); i++)
			{
				const DescriptorSetBinding& binding = this->setBindnings[i];
				const Util::Array<uint32_t>& offsets = this->setOffsets[i];
				dev->BindDescriptorsCompute(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size());
			}

			// update push ranges
			if (this->pushSize > 0)
			{
				dev->UpdatePushRanges(VK_SHADER_STAGE_COMPUTE_BIT, this->pushLayout, 0, this->pushSize, this->pushData);
			}
		}
	}
	else
	{
		// if no variation is being used, bind descriptors for both graphics and compute
		IndexT i;
		for (i = 0; i < this->setBindnings.Size(); i++)
		{
			const DescriptorSetBinding& binding = this->setBindnings[i];
			const Util::Array<uint32_t>& offsets = this->setOffsets[i];
			dev->BindDescriptorsGraphics(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size(), this->applyShared);
			dev->BindDescriptorsCompute(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size());
		}

		// push to both compute and graphics
		if (this->pushSize > 0)
		{
			dev->UpdatePushRanges(VK_SHADER_STAGE_ALL, this->pushLayout, 0, this->pushSize, this->pushData);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::AddDescriptorWrite(const VkWriteDescriptorSet& write)
{
	this->pendingSetWrites.Append(write);
	this->setsDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::SetupVariables(const Util::Array<IndexT>& groups)
{
	// get layouts associated with group and setup descriptor sets for them
	Util::FixedArray<VkDescriptorSetLayout> setLayouts;
	Util::FixedArray<bool> createOwnSet;

	// setup binds, we will use there later when applying the shader
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		DescriptorSetBinding binding;
		this->groupIndexMap.Add(groups[i], i);
#if AMD_DESC_SETS
		binding.set = this->sets[i];
#else
		binding.set = this->sets[this->shader->setToIndexMap[i]];
#endif
		binding.slot = groups[i];
		//binding.layout = this->shader->pipelineSetLayouts[groups[i]];
		binding.layout = this->shader->pipelineLayout;
		this->setBindnings[i] = binding;
	}

	/// setup variables for the groups this shader should modify
	for (i = 0; i < groups.Size(); i++)
	{
		const AnyFX::ProgramBase* program = this->effect->GetPrograms()[0];

		// get handles related to groups
		const eastl::vector<AnyFX::VariableBase*>& variables = this->effect->GetVariables(groups[i]);
		const eastl::vector<AnyFX::VarblockBase*>& varblocks = this->effect->GetVarblocks(groups[i]);
		const eastl::vector<AnyFX::VarbufferBase*>& varbuffers = this->effect->GetVarbuffers(groups[i]);

		// load uniforms
		uint j;
		for (j = 0; j < variables.size(); j++)
		{
			// get AnyFX variable
			AnyFX::VkVariable* variable = static_cast<AnyFX::VkVariable*>(variables[j]);

			// create new variable
			Ptr<ShaderVariable> var = ShaderVariable::Create();

			// setup variable from AnyFX variable
			var->Setup(variable, this, this->sets[i]);
			this->variables.Append(var);
			this->variablesByName.Add(variable->name.c_str(), var);
		}

		// load shader storage buffer variables
		for (j = 0; j < varblocks.size(); j++)
		{
			// get AnyFX variable
			AnyFX::VkVarblock* block = static_cast<AnyFX::VkVarblock*>(varblocks[j]);
			if (block->variables.empty() || AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push)) continue;

			// create new variable
			Ptr<ShaderVariable> var = ShaderVariable::Create();

			// setup variable from AnyFX varblock
			var->Setup(block, this, this->sets[i]);
			this->variables.Append(var);
			this->variablesByName.Add(block->name.c_str(), var);
		}

		// load uniform block variables
		for (j = 0; j < varbuffers.size(); j++)
		{
			// get varblock
			AnyFX::VkVarbuffer* buffer = static_cast<AnyFX::VkVarbuffer*>(varbuffers[j]);

			// create a new variable
			Ptr<ShaderVariable> var = ShaderVariable::Create();

			// setup variable from AnyFX varbuffer
			var->Setup(buffer, this, this->sets[i]);
			this->variables.Append(var);
			this->variablesByName.Add(buffer->name.c_str(), var);
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::SetupUniformBuffers(const Util::Array<IndexT>& groups)
{
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		// get varblocks by group
		const eastl::vector<AnyFX::VarblockBase*>& varblocks = this->effect->GetVarblocks(groups[i]);
		Util::Array<uint32_t> offsets;
		Util::Dictionary<uint32_t, BufferMapping> bufferMappings;
		uint32_t dynindex = 0;
		for (uint j = 0; j < varblocks.size(); j++)
		{
			AnyFX::VarblockBase* block = varblocks[j];

			bool usedBySystem = false;
			if (block->HasAnnotation("System")) usedBySystem = block->GetAnnotationBool("System");

			// only create a buffer if it's not system and only if it's being used in any of the shader programs
			if (!usedBySystem && block->alignedSize > 0)
			{
				// generate a name which we know will be unique
				Util::String name = block->name.c_str();

				// if we have an ordinary uniform buffer, allocate space for it
				if (!AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
				{
					n_assert(this->shader->buffers.Contains(name));
					Ptr<CoreGraphics::ConstantBuffer> uniformBuffer = this->shader->buffers[name];
					const Ptr<CoreGraphics::ShaderVariable>& bufferVar = this->GetVariableByName(name);

					// allocate an instance if buffer is marked for sub-buffer offsetting
					uint32_t instanceOffset = 0;
					if (block->Flag("DynamicOffset"))
					{
						// allocate single instance within uniform buffer and get offset
						instanceOffset = uniformBuffer->AllocateInstance();
						offsets.Append(instanceOffset);
						bufferMappings.Add(block->binding, BufferMapping{j, dynindex++});

						// add to dictionary so we can dealloc later
						this->instances.Add(uniformBuffer, instanceOffset);
					}					

					// update buffer
					for (uint k = 0; k < block->variables.size(); k++)
					{
						// find the shader variable and bind the constant buffer we just created to said variable
						const AnyFX::VariableBase* var = block->variables[k];
						Util::String name = var->name.c_str();
						unsigned varOffset = block->offsetsByName[var->name];
						const Ptr<CoreGraphics::ShaderVariable>& member = this->GetVariableByName(name);
						member->BindToUniformBuffer(uniformBuffer, instanceOffset + varOffset, var->byteSize, (int8_t*)var->currentValue);
					}

					// we apply the constant buffer again, in case we have to grow the buffer and reallocate it
					bufferVar->SetConstantBuffer(uniformBuffer);
				}
				else
				{
					// we only allow 1 push range
					n_assert(this->pushData == NULL);
					uint32_t size = VkRenderDevice::Instance()->deviceProps.limits.maxPushConstantsSize;

					// allocate push range
					this->pushData = n_new_array(uint8_t, size);
					this->pushSize = size;
					this->pushLayout = this->shader->pipelineLayout;
					for (uint k = 0; k < block->variables.size(); k++)
					{
						// find the shader variable and bind the constant buffer we just created to said variable
						const AnyFX::VariableBase* var = block->variables[k];
						Util::String name = var->name.c_str();
						unsigned varOffset = block->offsetsByName[var->name];
						const Ptr<CoreGraphics::ShaderVariable>& member = this->GetVariableByName(name);
						member->BindToPushConstantRange(this->pushData, varOffset, var->byteSize, (int8_t*)var->currentValue);
					}
				}
			}
			else if (block->Flag("DynamicOffset"))
			{
				offsets.Append(0);
				bufferMappings.Add(block->binding, BufferMapping{ j, dynindex++ });
			}
		}
		this->setOffsets[this->groupIndexMap[groups[i]]] = offsets;
		this->setBufferMapping[this->groupIndexMap[groups[i]]] = bufferMappings;
	}

	// perform descriptor set update, since our buffers might grow, we might have pending updates, and since the old buffer is destroyed, we want to flush all updates here.
	if (this->setsDirty) this->UpdateDescriptorSets();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::UpdateDescriptorSets()
{
	// first ensure descriptor sets are up to date with whatever the variable values has been set to
	// this can be destructive, because it changes the base shader state
	vkUpdateDescriptorSets(VkRenderDevice::dev, this->pendingSetWrites.Size(), this->pendingSetWrites.Begin(), 0, NULL);
	this->pendingSetWrites.Clear();
	this->setsDirty = false;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::Discard()
{
	if (this->pushData != NULL) n_delete_array(this->pushData);

	// free instances
	IndexT i;
	for (i = 0; i < this->instances.Size(); i++)
	{
		const Ptr<CoreGraphics::ConstantBuffer>& buf = this->instances.KeyAtIndex(i);
		buf->FreeInstance(this->instances.ValueAtIndex(i));
	}

	ShaderStateBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::SetDescriptorSet(const VkDescriptorSet& set, const IndexT slot)
{
	// update both references
	this->sets[groupIndexMap[slot]] = set;
	this->setBindnings[groupIndexMap[slot]].set = set;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::CreateOffsetArray(Util::Array<uint32_t>& outOffsets, const IndexT group)
{
	outOffsets = this->setOffsets[this->groupIndexMap[group]];
}

//------------------------------------------------------------------------------
/**
*/
VkShaderState::BufferMapping
VkShaderState::GetBufferMapping(const IndexT& group, const IndexT& binding)
{
	return this->setBufferMapping[this->groupIndexMap[group]][binding];
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Vulkan::VkShaderState::VkDerivativeState>
VkShaderState::CreateDerivative(const IndexT group)
{
	Ptr<VkDerivativeState> state = VkDerivativeState::Create();
	IndexT index = this->groupIndexMap[group];
	state->set = this->sets[index];
	state->group = group;
	state->layout = this->shader->pipelineLayout;
	state->parent = this;

	// get varblocks by group, only add dynamically offset buffers
	const eastl::vector<AnyFX::VarblockBase*>& varblocks = this->effect->GetVarblocks(group);
	for (uint32_t i = 0; i < varblocks.size(); i++)
	{
		const AnyFX::VarblockBase* block = varblocks[i];
		if (block->Flag("DynamicOffset")) state->buffers.Append(this->shader->buffersByGroup[group][i]);
	}
	return state;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::VkDerivativeState::Apply()
{
	n_assert(this->offsetCount == this->buffers.Size());
	uint32_t i;
	for (i = 0; i < this->offsetCount; i++)
	{
		this->buffers[i]->SetBaseOffset(this->offsets[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::VkDerivativeState::Commit()
{
	VkRenderDevice* dev = VkRenderDevice::Instance();
	if (this->parent->setsDirty) this->parent->UpdateDescriptorSets();
	switch (this->bindPoint)
	{
	case VK_PIPELINE_BIND_POINT_GRAPHICS:
		dev->BindDescriptorsGraphics(&this->set, this->layout, this->group, 1, this->offsets, this->offsetCount, this->bindShared);
		break;
	case VK_PIPELINE_BIND_POINT_COMPUTE:
		dev->BindDescriptorsCompute(&this->set, this->layout, this->group, 1, this->offsets, this->offsetCount);
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::VkDerivativeState::Reset()
{
	SizeT i;
	for (i = 0; i < this->buffers.Size(); i++)
	{
		this->buffers[i]->SetBaseOffset(0);
	}
}

} // namespace Vulkan