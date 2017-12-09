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
VkShaderState::VkShaderState()
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
VkShaderState::Setup(
	const Ids::Id24 id, 
	AnyFX::ShaderEffect* effect, 
	const Util::Array<IndexT>& groups, 
	ShaderStateAllocator& allocator, 
	Util::FixedArray<VkDescriptorSet>& sets,
	Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
	bool createUniqueSet)
{
	// copy effect pointer
	allocator.Get<0>(id) = effect;
	SetupInfo& setup = allocator.Get<1>(id);
	RuntimeInfo& runtime = allocator.Get<2>(id);
	VkShaderVariable::ShaderVariableAllocator& varAllocator = allocator.Get<3>(id);

	// copy sets from shader
	setup.sets.Resize(groups.Size());
	setup.setBufferMapping.Resize(groups.Size());
	runtime.setBindings.Resize(groups.Size());
	runtime.setOffsets.Resize(groups.Size());
	runtime.setsDirty = true;
	Util::FixedArray<bool> isActive(groups.Size());
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		bool hasSet = sets.Size() > groups[i];
		if (hasSet)
		{
			setup.sets[i] = sets[groups[i]];
			isActive[i] = true;
		}	
		else
		{
			isActive[i] = false;
			setup.sets[i] = VK_NULL_HANDLE;
		}
	}

	// if we want to create our own resource set
	if (createUniqueSet)
	{
		for (i = 0; i < groups.Size(); i++)
		{	
			if (isActive[i])
			{
				VkDescriptorSetLayout layout = setLayouts[groups[i]];

				// allocate descriptor sets
				VkDescriptorSetAllocateInfo info =
				{
					VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					NULL,
					VkRenderDevice::descPool,
					1,
					&layout
				};
				VkResult res = vkAllocateDescriptorSets(VkRenderDevice::dev, &info, &sets[i]);
				n_assert(res == VK_SUCCESS);
			}			
		}
	}


	// setup variables if we have any layouts
	if (!setLayouts.IsEmpty())
	{
		VkShaderState::SetupVariables(effect, runtime, setup, varAllocator, groups);
		VkShaderState::SetupUniformBuffers(effect, runtime, setup, varAllocator, groups);
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
VkShaderState::Commit(Ids::Id24 currentProgram, Util::Array<VkWriteDescriptorSet>& writes, bool& setsDirty, RuntimeInfo& info)
{
	if (setsDirty) VkShaderState::UpdateDescriptorSets(writes, setsDirty);

	// get render device to apply state
	VkRenderDevice* dev = VkRenderDevice::Instance();

	// now go through and make sure the shader can bind the sets updated
	if (currentProgram != Ids::InvalidId24 && dev->currentBindPoint != VkShaderProgram::InvalidType)
	{
		const VkShaderProgram::PipelineType type = dev->currentBindPoint;
		n_assert(type != VkShaderProgram::InvalidType);
		if (type == VkShaderProgram::Graphics)
		{
			// if no variation is being used, bind descriptors for both graphics and compute
			IndexT i;
			for (i = 0; i < info.setBindings.Size(); i++)
			{
				const DescriptorSetBinding& binding = info.setBindings[i];
				const Util::Array<uint32_t>& offsets = info.setOffsets[i];
				dev->BindDescriptorsGraphics(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size(), info.shared);
			}

			// update push ranges
			if (info.pushDataSize > 0)
			{
				dev->UpdatePushRanges(VK_SHADER_STAGE_ALL_GRAPHICS, info.pushLayout, 0, info.pushDataSize, info.pushData);
			}
		}
		else
		{
			// if no variation is being used, bind descriptors for both graphics and compute
			IndexT i;
			for (i = 0; i < info.setBindings.Size(); i++)
			{
				const DescriptorSetBinding& binding = info.setBindings[i];
				const Util::Array<uint32_t>& offsets = info.setOffsets[i];
				dev->BindDescriptorsCompute(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size());
			}

			// update push ranges
			if (info.pushDataSize > 0)
			{
				dev->UpdatePushRanges(VK_SHADER_STAGE_COMPUTE_BIT, info.pushLayout, 0, info.pushDataSize, info.pushData);
			}
		}
	}
	else
	{
		// if no variation is being used, bind descriptors for both graphics and compute
		IndexT i;
		for (i = 0; i < info.setBindings.Size(); i++)
		{
			const DescriptorSetBinding& binding = info.setBindings[i];
			const Util::Array<uint32_t>& offsets = info.setOffsets[i];
			dev->BindDescriptorsGraphics(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size(), info.shared);
			dev->BindDescriptorsCompute(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size());
		}

		// push to both compute and graphics
		if (info.pushDataSize > 0)
		{
			dev->UpdatePushRanges(VK_SHADER_STAGE_ALL, info.pushLayout, 0, info.pushDataSize, info.pushData);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::SetupVariables(AnyFX::ShaderEffect* effect, RuntimeInfo& runtime, SetupInfo& setup, VkShaderVariable::ShaderVariableAllocator& varAllocator, const Util::Array<IndexT>& groups)
{
	// get layouts associated with group and setup descriptor sets for them
	Util::FixedArray<VkDescriptorSetLayout> setLayouts;
	Util::FixedArray<bool> createOwnSet;

	// setup binds, we will use there later when applying the shader
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		DescriptorSetBinding binding;
		setup.groupIndexMap.Add(groups[i], i);
#if AMD_DESC_SETS
		binding.set = setup.sets[i];
#else
		binding.set = setup.sets[this->shader->setToIndexMap[i]];
#endif
		binding.slot = groups[i];
		//binding.layout = this->shader->pipelineSetLayouts[groups[i]];
		binding.layout = setup.pipelineLayout;
		runtime.setBindings[i] = binding;
	}

	/// setup variables for the groups this shader should modify
	for (i = 0; i < groups.Size(); i++)
	{
		const AnyFX::ProgramBase* program = effect->GetPrograms()[0];

		// get handles related to groups
		const eastl::vector<AnyFX::VariableBase*>& variables = effect->GetVariables(groups[i]);
		const eastl::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks(groups[i]);
		const eastl::vector<AnyFX::VarbufferBase*>& varbuffers = effect->GetVarbuffers(groups[i]);

		// load uniforms
		uint j;
		for (j = 0; j < variables.size(); j++)
		{
			// get AnyFX variable
			AnyFX::VkVariable* variable = static_cast<AnyFX::VkVariable*>(variables[j]);

			Ids::Id24 varId = varAllocator.AllocObject();
			VkShaderVariable::Setup(variable, varId, varAllocator, setup.sets[i]);
			setup.variableMap.Add(variable->name.c_str(), varId);
		}

		// load shader storage buffer variables
		for (j = 0; j < varblocks.size(); j++)
		{
			// get AnyFX variable
			AnyFX::VkVarblock* block = static_cast<AnyFX::VkVarblock*>(varblocks[j]);
			if (block->variables.empty() || AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push)) continue;

			Ids::Id24 varId = varAllocator.AllocObject();
			VkShaderVariable::Setup(block, varId, varAllocator, setup.sets[i]);
			setup.variableMap.Add(block->name.c_str(), varId);
		}

		// load uniform block variables
		for (j = 0; j < varbuffers.size(); j++)
		{
			// get varblock
			AnyFX::VkVarbuffer* buffer = static_cast<AnyFX::VkVarbuffer*>(varbuffers[j]);

			Ids::Id24 varId = varAllocator.AllocObject();
			VkShaderVariable::Setup(buffer, varId, varAllocator, setup.sets[i]);
			setup.variableMap.Add(buffer->name.c_str(), varId);
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderState::SetupUniformBuffers(AnyFX::ShaderEffect* effect, RuntimeInfo& runtime, SetupInfo& setup, VkShaderVariable::ShaderVariableAllocator& varAllocator, const Util::Array<IndexT>& groups)
{
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		// get varblocks by group
		const eastl::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks(groups[i]);
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
VkShaderState::UpdateDescriptorSets(Util::Array<VkWriteDescriptorSet>& writes, bool& dirty)
{
	// first ensure descriptor sets are up to date with whatever the variable values has been set to
	// this can be destructive, because it changes the base shader state
	vkUpdateDescriptorSets(VkRenderDevice::dev, writes.Size(), writes.Begin(), 0, NULL);
	writes.Clear();
	dirty = false;
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