//------------------------------------------------------------------------------
// vkshaderstate.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderstate.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shader.h"
#include "vkgraphicsdevice.h"
#include "vkshaderconstant.h"
#include "vkconstantbuffer.h"
#include "vkshaderprogram.h"
#include "coregraphics/shaderpool.h"
#include "vkresourcetable.h"

using namespace CoreGraphics;
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetup(
	const CoreGraphics::ShaderStateId id,
	AnyFX::ShaderEffect* effect, 
	const Util::Array<IndexT>& groups, 
	VkShaderStateAllocator& allocator,
	Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& sharedBuffers,
	Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& sharedBuffersByGroup,
	CoreGraphics::ResourcePipelineId layout)
{
	// copy effect pointer
	allocator.Get<0>(id.stateId) = effect;
	VkShaderStateRuntimeInfo& runtime = allocator.Get<1>(id.stateId);
	VkShaderStateSetupInfo& setup = allocator.Get<2>(id.stateId);
	VkShaderConstantAllocator& varAllocator = allocator.Get<3>(id.stateId);

	// copy sets from shader
	setup.sets.Resize(groups.Size());
	setup.setBufferMapping.Resize(groups.Size());
	setup.descPool = Vulkan::GetCurrentDescriptorPool();
	setup.pipelineLayout = layout;
	runtime.dev = Vulkan::GetCurrentDevice();
	runtime.setBindings.Resize(groups.Size());
	runtime.setOffsets.Resize(groups.Size());
	runtime.setsDirty = true;


	// if we want to create our own resource set
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{	
		ResourceTableCreateInfo tinfo =
		{
			std::get<1>(setLayouts[groups[i]])
		};
		setup.sets[i] = CreateResourceTable(tinfo);
	}

	// setup variables if we have any layouts
	if (!setLayouts.IsEmpty())
	{
		VkShaderStateSetupConstants(id, effect, runtime, setup, varAllocator, groups);
		VkShaderStateSetupConstantBuffers(id, effect, runtime, setup, varAllocator, groups, sharedBuffers, sharedBuffersByGroup);
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
	const std::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks((const unsigned)group);
	for (uint32_t i = 0; i < varblocks.size(); i++)
	{
		const AnyFX::VarblockBase* block = varblocks[i];
		if (block->set == NEBULAT_DYNAMIC_OFFSET_GROUP) info.buffers.Append(buffersByGroup[group][i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateCommit(VkShaderStateRuntimeInfo& stateInfo)
{
	IndexT i;
	for (i = 0; i < stateInfo.setBindings.Size(); i++)
	{
		ResourceTableCommitChanges(stateInfo.setBindings[i].set);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateSetupConstants(const CoreGraphics::ShaderStateId id, AnyFX::ShaderEffect* effect, VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, VkShaderConstantAllocator& varAllocator, const Util::Array<IndexT>& groups)
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
		const std::vector<AnyFX::VariableBase*>& variables = effect->HasVariables(groups[i]) ? effect->GetVariables(groups[i]) : std::vector<AnyFX::VariableBase*>();
		const std::vector<AnyFX::VarblockBase*>& varblocks = effect->HasVarblocks(groups[i]) ? effect->GetVarblocks(groups[i]) : std::vector<AnyFX::VarblockBase*>();
		const std::vector<AnyFX::VarbufferBase*>& varbuffers = effect->HasVarbuffers(groups[i]) ? effect->GetVarbuffers(groups[i]) : std::vector<AnyFX::VarbufferBase*>();

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
VkShaderStateSetupConstantBuffers(const CoreGraphics::ShaderStateId id, AnyFX::ShaderEffect* effect, VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, VkShaderConstantAllocator& varAllocator, const Util::Array<IndexT>& groups, Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& sharedBuffers, Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& sharedBuffersByGroup)
{
	IndexT i;
	for (i = 0; i < groups.Size(); i++)
	{
		// get varblocks by group
		const std::vector<AnyFX::VarblockBase*>& varblocks = effect->GetVarblocks(groups[i]);
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
					const CoreGraphics::ShaderConstantId bufferVar = ShaderStateGetConstant(id, name);
					CoreGraphics::ConstantBufferId uniformBuffer;

					ConstantBufferCreateInfo crInfo =
					{
						false,
						ShaderId::Invalid(),
						name,
						block->alignedSize,
						1
					};

					// allocate an instance if buffer is marked for sub-buffer offsetting
					if (block->set == NEBULAT_DYNAMIC_OFFSET_GROUP)
					{
						// if we don't have a buffer here, create one
						IndexT bufIdx = sharedBuffers.FindIndex(name);
						if (bufIdx == InvalidIndex)
						{
							uniformBuffer = CreateConstantBuffer(crInfo);

							// update shader
							sharedBuffers.Add(name, uniformBuffer);
							Util::Array<CoreGraphics::ConstantBufferId>& group = sharedBuffersByGroup.AddUnique(block->set);
							group.Append(uniformBuffer);
						}
						else
						{
							uniformBuffer = sharedBuffers.ValueAtIndex(bufIdx);
						}

						// allocate single instance within uniform buffer and get offset
						CoreGraphics::ConstantBufferSliceId instanceOffset = ConstantBufferAllocateInstance(uniformBuffer);
						offsets.Append(instanceOffset.id);
						bufferMappings.Add(block->binding, VkShaderStateBufferMapping{j, dynindex++});

						// add to dictionary so we can dealloc later
						setup.instances.Add(uniformBuffer, instanceOffset);
					}			
					else
					{
						// buffer is not dynamically bound, so this state creates its own backing of that buffer
						uniformBuffer = CreateConstantBuffer(crInfo);
						setup.bufferMap.Add(name, uniformBuffer);
					}

					// setup variables in this state related to those buffers
					for (uint k = 0; k < block->variables.size(); k++)
					{
						// find the shader variable and bind the constant buffer we just created to said variable
						const AnyFX::VariableBase* var = block->variables[k];
						Util::StringAtom varname = var->name.c_str();
						unsigned varOffset = block->offsetsByName[varname.Value()];
						const CoreGraphics::ShaderConstantId member = ShaderStateGetConstant(id, varname);
						VkShaderConstantBindToUniformBuffer(member, uniformBuffer, varAllocator, varOffset, var->byteSize, (int8_t*)var->currentValue);
					}

					// we apply the constant buffer again, in case we have to grow the buffer and reallocate it
					VkShaderConstantDescriptorBinding& resource = varAllocator.Get<1>(bufferVar.id);
					SetConstantBuffer(resource, uniformBuffer);
				}
				else
				{
					// we only allow 1 push range
					n_assert(runtime.pushData == nullptr);
					
					uint32_t size = Vulkan::GetCurrentProperties().limits.maxPushConstantsSize;

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
			else if (block->set == NEBULAT_DYNAMIC_OFFSET_GROUP)
			{
				offsets.Append(0);
				bufferMappings.Add(block->binding, VkShaderStateBufferMapping{ j, dynindex++ });
			}
		}
		runtime.setOffsets[setup.groupIndexMap[groups[i]]] = offsets;
		setup.setBufferMapping[setup.groupIndexMap[groups[i]]] = bufferMappings;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStateDiscard(VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, VkShaderConstantAllocator& varAllocator)
{
	if (runtime.pushData != nullptr) n_delete_array(runtime.pushData);

	IndexT i;
	for (i = 0; i < setup.sets.Size(); i++)
		DestroyResourceTable(setup.sets[i]);

	// free instances
	for (i = 0; i < setup.instances.Size(); i++)
	{
		const CoreGraphics::ConstantBufferId buf = setup.instances.KeyAtIndex(i);
		ConstantBufferFreeInstance(setup.instances.KeyAtIndex(i), setup.instances.ValueAtIndex(i));
	}

	for (i = 0; i < setup.bufferMap.Size(); i++)
		DestroyConstantBuffer(setup.bufferMap.ValueAtIndex(i));

	for (i = 0; i < setup.variableMap.Size(); i++)
		varAllocator.DeallocObject(setup.variableMap.ValueAtIndex(i).id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ResourceTableId&
VkShaderStateGetResourceTable(CoreGraphics::ShaderStateId id, const IndexT group)
{
	VkShaderStateAllocator& stateAlloc = CoreGraphics::shaderPool->GetStateAllocator(id);
	VkShaderStateRuntimeInfo& rtinfo = stateAlloc.Get<1>(id.stateId);
	VkShaderStateSetupInfo& setupInfo = stateAlloc.Get<2>(id.stateId);
	return rtinfo.setBindings[setupInfo.groupIndexMap[group]].set;
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
	switch (info.bindPoint)
	{
	case VK_PIPELINE_BIND_POINT_GRAPHICS:
		Vulkan::BindDescriptorsGraphics(
			&ResourceTableGetVkDescriptorSet(info.set),
			ResourcePipelineGetVk(info.layout), 
			info.group, 
			1, 
			info.offsets, 
			info.offsetCount, 
			info.bindShared);
		break;
	case VK_PIPELINE_BIND_POINT_COMPUTE:
		Vulkan::BindDescriptorsCompute(
			&ResourceTableGetVkDescriptorSet(info.set),
			ResourcePipelineGetVk(info.layout), 
			info.group, 
			1, 
			info.offsets, 
			info.offsetCount);
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
VkShaderStateSetPropagate(CoreGraphics::ShaderStateId id, bool b)
{
	VkShaderStateRuntimeInfo& rtinfo = CoreGraphics::shaderPool->GetStateAllocator(id).Get<1>(id.stateId);
	rtinfo.propagate = b;
}

} // namespace Vulkan