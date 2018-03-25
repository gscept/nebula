//------------------------------------------------------------------------------
// vkshaderstate.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderstate.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shader.h"
#include "vkrenderdevice.h"
#include "vkshaderconstant.h"
#include "vkconstantbuffer.h"
#include "vkshaderprogram.h"
#include "coregraphics/shaderpool.h"

using namespace CoreGraphics;
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetup(
	const Ids::Id24 id, 
	AnyFX::ShaderEffect* effect, 
	const Util::Array<IndexT>& groups, 
	VkShaderStateAllocator& allocator,
	Util::FixedArray<VkDescriptorSet>& sets,
	Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
	const Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
	bool createUniqueSet)
{
	// copy effect pointer
	allocator.Get<0>(id) = effect;
	VkShaderStateRuntimeInfo& runtime = allocator.Get<1>(id);
	VkShaderStateSetupInfo& setup = allocator.Get<2>(id);	
	VkShaderConstantAllocator& varAllocator = allocator.Get<3>(id);
	Util::Array<VkWriteDescriptorSet>& setWrites = allocator.Get<4>(id);

	// copy sets from shader
	setup.sets.Resize(groups.Size());
	setup.setBufferMapping.Resize(groups.Size());
	setup.descPool = VkRenderDevice::Instance()->GetCurrentDescriptorPool();
	setup.freeSet = createUniqueSet;
	runtime.dev = VkRenderDevice::Instance()->GetCurrentDevice();
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
					nullptr,
					setup.descPool,
					1,
					&layout
				};
				VkResult res = vkAllocateDescriptorSets(runtime.dev, &info, &sets[i]);
				n_assert(res == VK_SUCCESS);
			}			
		}
	}

	// setup variables if we have any layouts
	if (!setLayouts.IsEmpty())
	{
		VkShaderStateSetupConstants(id, effect, runtime, setup, varAllocator, groups);
		VkShaderStateSetupConstantBuffers(id, effect, runtime, setup, varAllocator, setWrites, groups, buffers);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetupDerivative(const Ids::Id32 id, const AnyFX::ShaderEffect* effect, VkDerivativeStateAllocator& allocator, VkShaderStateRuntimeInfo& parentRuntime, VkShaderStateSetupInfo& parentSetup, const Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& buffersByGroup, const IndexT group)
{
	VkDerivativeShaderStateRuntimeInfo& info = allocator.Get<0>(id);
	IndexT index = parentSetup.groupIndexMap[group];

	info.layout = parentSetup.pipelineLayout;
	info.set = parentSetup.sets[index];
	info.group = group;
	info.parentRuntime = &parentRuntime;
	info.parentSetup = &parentSetup;

	// get varblocks by group, only add dynamically offset buffers
	const eastl::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks((const unsigned)group);
	for (uint32_t i = 0; i < varblocks.size(); i++)
	{
		const AnyFX::VarblockBase* block = varblocks[i];
		if (block->Flag("DynamicOffset")) info.buffers.Append(buffersByGroup[group][i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateCommit(Ids::Id24 currentProgram, Util::Array<VkWriteDescriptorSet>& writes, VkShaderStateRuntimeInfo& stateInfo)
{
	if (stateInfo.setsDirty) VkShaderStateUpdateDescriptorSets(stateInfo.dev, writes, stateInfo.setsDirty);

	// get render device to apply state
	VkRenderDevice* dev = VkRenderDevice::Instance();

	// now go through and make sure the shader can bind the sets updated
	if (currentProgram != Ids::InvalidId24 && dev->currentBindPoint != VkShaderProgramPipelineType::InvalidType)
	{
		const VkShaderProgramPipelineType type = dev->currentBindPoint;
		n_assert(type != VkShaderProgramPipelineType::InvalidType);
		if (type == VkShaderProgramPipelineType::GraphicsPipeline)
		{
			// if no variation is being used, bind descriptors for both graphics and compute
			IndexT i;
			for (i = 0; i < stateInfo.setBindings.Size(); i++)
			{
				const VkShaderStateDescriptorSetBinding& binding = stateInfo.setBindings[i];
				const Util::Array<uint32_t>& offsets = stateInfo.setOffsets[i];
				dev->BindDescriptorsGraphics(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size(), stateInfo.shared);
			}

			// update push ranges
			if (stateInfo.pushDataSize > 0)
			{
				dev->UpdatePushRanges(VK_SHADER_STAGE_ALL_GRAPHICS, stateInfo.pushLayout, 0, stateInfo.pushDataSize, stateInfo.pushData);
			}
		}
		else
		{
			// if no variation is being used, bind descriptors for both graphics and compute
			IndexT i;
			for (i = 0; i < stateInfo.setBindings.Size(); i++)
			{
				const VkShaderStateDescriptorSetBinding& binding = stateInfo.setBindings[i];
				const Util::Array<uint32_t>& offsets = stateInfo.setOffsets[i];
				dev->BindDescriptorsCompute(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size());
			}

			// update push ranges
			if (stateInfo.pushDataSize > 0)
			{
				dev->UpdatePushRanges(VK_SHADER_STAGE_COMPUTE_BIT, stateInfo.pushLayout, 0, stateInfo.pushDataSize, stateInfo.pushData);
			}
		}
	}
	else
	{
		// if no variation is being used, bind descriptors for both graphics and compute
		IndexT i;
		for (i = 0; i < stateInfo.setBindings.Size(); i++)
		{
			const VkShaderStateDescriptorSetBinding& binding = stateInfo.setBindings[i];
			const Util::Array<uint32_t>& offsets = stateInfo.setOffsets[i];
			dev->BindDescriptorsGraphics(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size(), stateInfo.shared);
			dev->BindDescriptorsCompute(&binding.set, binding.layout, binding.slot, 1, offsets.Begin(), offsets.Size());
		}

		// push to both compute and graphics
		if (stateInfo.pushDataSize > 0)
		{
			dev->UpdatePushRanges(VK_SHADER_STAGE_ALL, stateInfo.pushLayout, 0, stateInfo.pushDataSize, stateInfo.pushData);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetupConstants(const Ids::Id24 id, AnyFX::ShaderEffect* effect, VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, VkShaderConstantAllocator& varAllocator, const Util::Array<IndexT>& groups)
{
	// setup binds, we will use there later when applying the shader
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		VkShaderStateDescriptorSetBinding binding;
		setup.groupIndexMap.Add(groups[i], i);
		binding.set = setup.sets[i];
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
			VkShaderConstantSetup(variable, varId, varAllocator, setup.sets[i]);
			ShaderConstantId cid;
			cid.id = varId;
			setup.variableMap.Add(variable->name.c_str(), cid);
		}

		// load shader storage buffer variables
		for (j = 0; j < varblocks.size(); j++)
		{
			// get AnyFX variable
			AnyFX::VkVarblock* block = static_cast<AnyFX::VkVarblock*>(varblocks[j]);
			if (block->variables.empty() || AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push)) continue;

			Ids::Id24 varId = varAllocator.AllocObject();
			VkShaderConstantSetup(block, varId, varAllocator, setup.sets[i]);
			ShaderConstantId cid;
			cid.id = varId;
			setup.variableMap.Add(block->name.c_str(), cid);
		}

		// load uniform block variables
		for (j = 0; j < varbuffers.size(); j++)
		{
			// get varblock
			AnyFX::VkVarbuffer* buffer = static_cast<AnyFX::VkVarbuffer*>(varbuffers[j]);

			Ids::Id24 varId = varAllocator.AllocObject();
			VkShaderConstantSetup(buffer, varId, varAllocator, setup.sets[i]);
			ShaderConstantId cid;
			cid.id = varId;
			setup.variableMap.Add(buffer->name.c_str(), cid);
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetupConstantBuffers(const Ids::Id24 id, AnyFX::ShaderEffect* effect, VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, VkShaderConstantAllocator& varAllocator, Util::Array<VkWriteDescriptorSet>& setWrites, const Util::Array<IndexT>& groups, const Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers)
{
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		// get varblocks by group
		const eastl::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks(groups[i]);
		Util::Array<uint32_t> offsets;
		Util::Dictionary<uint32_t, VkShaderStateBufferMapping> bufferMappings;
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
				Util::StringAtom name = block->name.c_str();

				// if we have an ordinary uniform buffer, allocate space for it
				if (!AnyFX::HasFlags(block->qualifiers, AnyFX::Qualifiers::Push))
				{
					n_assert(buffers.Contains(name));
					CoreGraphics::ConstantBufferId uniformBuffer = buffers[name];
					const CoreGraphics::ShaderConstantId bufferVar = ShaderStateGetConstant(id, name);

					// allocate an instance if buffer is marked for sub-buffer offsetting
					CoreGraphics::ConstantBufferSliceId instanceOffset = CoreGraphics::ConstantBufferSliceId::Invalid();
					if (block->Flag("DynamicOffset"))
					{
						// allocate single instance within uniform buffer and get offset
						instanceOffset = ConstantBufferAllocateInstance(uniformBuffer);
						offsets.Append(instanceOffset.id);
						bufferMappings.Add(block->binding, VkShaderStateBufferMapping{j, dynindex++});

						// add to dictionary so we can dealloc later
						setup.instances.Add(uniformBuffer, instanceOffset);
					}					

					// setup variables in this state related to those buffers
					for (uint k = 0; k < block->variables.size(); k++)
					{
						// find the shader variable and bind the constant buffer we just created to said variable
						const AnyFX::VariableBase* var = block->variables[k];
						Util::StringAtom name = var->name.c_str();
						unsigned varOffset = block->offsetsByName[var->name];
						const CoreGraphics::ShaderConstantId member = ShaderStateGetConstant(id, name);
						VkShaderConstantBindToUniformBuffer(member, uniformBuffer, varAllocator, varOffset, var->byteSize, (int8_t*)var->currentValue);
					}

					// we apply the constant buffer again, in case we have to grow the buffer and reallocate it
					VkShaderConstantDescriptorBinding& resource = varAllocator.Get<1>(bufferVar.id);
					SetConstantBuffer(resource, setWrites, uniformBuffer);
				}
				else
				{
					// we only allow 1 push range
					n_assert(runtime.pushData == nullptr);
					
					uint32_t size = VkRenderDevice::Instance()->GetCurrentProperties().limits.maxPushConstantsSize;

					// allocate push range
					runtime.pushData = n_new_array(uint8_t, size);
					runtime.pushDataSize = size;
					runtime.pushLayout = setup.pipelineLayout;
					for (uint k = 0; k < block->variables.size(); k++)
					{
						// find the shader variable and bind the constant buffer we just created to said variable
						const AnyFX::VariableBase* var = block->variables[k];
						Util::String name = var->name.c_str();
						unsigned varOffset = block->offsetsByName[var->name];
						const CoreGraphics::ShaderConstantId member = ShaderStateGetConstant(id, name);
						VkShaderConstantBindToPushConstantRange(member, runtime.pushData, varAllocator, varOffset, var->byteSize, (int8_t*)var->currentValue);
					}
				}
			}
			else if (block->Flag("DynamicOffset"))
			{
				offsets.Append(0);
				bufferMappings.Add(block->binding, VkShaderStateBufferMapping{ j, dynindex++ });
			}
		}
		runtime.setOffsets[setup.groupIndexMap[groups[i]]] = offsets;
		setup.setBufferMapping[setup.groupIndexMap[groups[i]]] = bufferMappings;
	}

	// perform descriptor set update, since our buffers might grow, we might have pending updates, and since the old buffer is destroyed, we want to flush all updates here.
	if (runtime.setsDirty) VkShaderStateUpdateDescriptorSets(runtime.dev, setWrites, runtime.setsDirty);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateDiscard(const Ids::Id24 id, VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, VkShaderConstantAllocator& varAllocator)
{
	if (runtime.pushData != nullptr) n_delete_array(runtime.pushData);

	if (setup.freeSet)
	{
		if (setup.sets.Size() > 0)
			vkFreeDescriptorSets(runtime.dev, setup.descPool, setup.sets.Size(), setup.sets.Begin());
	}

	// free instances
	IndexT i;
	for (i = 0; i < setup.instances.Size(); i++)
	{
		const CoreGraphics::ConstantBufferId buf = setup.instances.KeyAtIndex(i);
		ConstantBufferFreeInstance(setup.instances.KeyAtIndex(i), setup.instances.ValueAtIndex(i));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateUpdateDescriptorSets(VkDevice dev, Util::Array<VkWriteDescriptorSet>& writes, bool& dirty)
{
	// first ensure descriptor sets are up to date with whatever the variable values has been set to
	// this can be destructive, because it changes the base shader state
	vkUpdateDescriptorSets(dev, writes.Size(), writes.Begin(), 0, nullptr);
	writes.Clear();
	dirty = false;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetDescriptorSet(VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, const VkDescriptorSet& set, const IndexT slot)
{
	// update both references
	setup.sets[setup.groupIndexMap[slot]] = set;
	runtime.setBindings[setup.groupIndexMap[slot]].set = set;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateCreateOffsetArray(VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, Util::Array<uint32_t>& outOffsets, const IndexT group)
{
	outOffsets = runtime.setOffsets[setup.groupIndexMap[group]];
}

//------------------------------------------------------------------------------
/**
*/
VkShaderStateBufferMapping
VkShaderStateGetBufferMapping(VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, const IndexT& group, const IndexT& binding)
{
	return setup.setBufferMapping[setup.groupIndexMap[group]][binding];
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateDerivativeStateApply(const VkDerivativeShaderStateRuntimeInfo& info)
{
	n_assert(info.offsetCount == info.buffers.Size());
	uint32_t i;
	for (i = 0; i < info.offsetCount; i++)
	{
		ConstantBufferSetBaseOffset(info.buffers[i], info.offsets[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateDerivativeStateCommit(const VkDerivativeShaderStateRuntimeInfo& info)
{
	VkRenderDevice* dev = VkRenderDevice::Instance();
	switch (info.bindPoint)
	{
	case VK_PIPELINE_BIND_POINT_GRAPHICS:
		dev->BindDescriptorsGraphics(&info.set, info.layout, info.group, 1, info.offsets, info.offsetCount, info.bindShared);
		break;
	case VK_PIPELINE_BIND_POINT_COMPUTE:
		dev->BindDescriptorsCompute(&info.set, info.layout, info.group, 1, info.offsets, info.offsetCount);
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateDerivativeStateReset(VkDerivativeShaderStateRuntimeInfo& info)
{
	IndexT i;
	for (i = 0; i < info.buffers.Size(); i++) ConstantBufferSetBaseOffset(info.buffers[0], 0);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetDescriptorSet(CoreGraphics::ShaderStateId id, VkDescriptorSet set, const IndexT group)
{
	VkShaderStateRuntimeInfo& rtinfo = CoreGraphics::shaderPool->GetStateAllocator(id).Get<1>(id.stateId);
	rtinfo.setBindings[group].set = set;

}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetShared(CoreGraphics::ShaderStateId id, bool b)
{
	VkShaderStateRuntimeInfo& rtinfo = CoreGraphics::shaderPool->GetStateAllocator(id).Get<1>(id.stateId);
	rtinfo.shared = b;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateAddDescriptorSetWrite(CoreGraphics::ShaderStateId id, VkWriteDescriptorSet write)
{
	CoreGraphics::shaderPool->GetStateAllocator(id).Get<4>(id.stateId).Append(write);
}

} // namespace Vulkan