#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader instance (local variables and such) in Vulkan.

	To get the shader state to be modified, one has to modify the value of a shader variable.
	To get the shader state to apply a specific descriptor set (0 is enabled by default), 
	one has to explain that the shader state should commit that descriptor set.

	The VkShaderState can create a derivative state, which is a derivation of a single
	group (descriptor set) and a set of offsets which are not stored in the set. The way
	to use it is like this:
	1. Create derivative set.
	2. Set offset pointer.
	3. Apply derivative.
		3.5 Update variables using shader state (variables retrieved from it)
	4. Commit derivative.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/constantbuffer.h"
#include "vkshaderconstant.h"
#include "lowlevel/afxapi.h"
namespace Lighting
{
	class VkLightServer;
}
namespace Vulkan
{

#pragma pack(push, 16)
struct VkShaderStateDescriptorSetBinding
{
	VkDescriptorSet set;
	VkPipelineLayout layout;
	IndexT slot;
};

struct VkShaderStateBufferMapping
{
	uint32_t index;
	uint32_t offset;
};
#pragma pack(pop)

struct VkShaderStateRuntimeInfo
{
	VkDevice dev;
	Util::FixedArray<VkShaderStateDescriptorSetBinding> setBindings;
	Util::FixedArray<Util::Array<uint32_t>> setOffsets;
	VkPipelineLayout pushLayout;
	uint8_t* pushData;
	uint32_t pushDataSize;
	bool setsDirty;
	bool shared;
};

struct VkShaderStateSetupInfo
{
	VkDescriptorPool descPool;
	bool freeSet;
	Util::FixedArray<VkDescriptorSet> sets;
	Util::FixedArray<Util::Dictionary<uint32_t, VkShaderStateBufferMapping>> setBufferMapping;
	Util::Dictionary<uint32_t, uint32_t> groupIndexMap;
	Util::Array<uint32_t> offsets;
	Util::Dictionary<Util::StringAtom, uint32_t> offsetsByName;
	Util::Dictionary<CoreGraphics::ConstantBufferId, CoreGraphics::ConstantBufferSliceId> instances;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderConstantId> variableMap;
	VkPipelineLayout pipelineLayout;
};

struct VkDerivativeShaderStateRuntimeInfo
{
	VkPipelineBindPoint bindPoint;
	uint32_t offsetCount;
	uint32_t* offsets;
	Util::Array<CoreGraphics::ConstantBufferId> buffers;
	uint32_t group;
	VkDescriptorSet set;
	VkPipelineLayout layout;
	bool bindShared;
	VkShaderStateRuntimeInfo* parentRuntime;
	VkShaderStateSetupInfo* parentSetup;
};

typedef Ids::IdAllocator<VkDerivativeShaderStateRuntimeInfo> VkDerivativeStateAllocator;

typedef Ids::IdAllocator<
	AnyFX::ShaderEffect*,												//0 effect
	VkShaderStateRuntimeInfo,											//1 setup info
	VkShaderStateSetupInfo,												//2 runtime info
	VkShaderConstantAllocator,											//3 variable allocator
	Util::Array<VkWriteDescriptorSet>,									//4 descriptor set writes
	VkDerivativeStateAllocator											//5 derivative states
> VkShaderStateAllocator;

/// setup the shader instance from its original shader object
void VkShaderStateSetup(
	const Ids::Id24 id,
	AnyFX::ShaderEffect* effect,
	const Util::Array<IndexT>& groups,
	VkShaderStateAllocator& allocator,
	Util::FixedArray<VkDescriptorSet>& sets,
	Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
	const Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
	bool createUniqueSet);

/// setup derivative
void VkShaderStateSetupDerivative(
	const Ids::Id32 id,
	const AnyFX::ShaderEffect* effect,
	VkDerivativeStateAllocator& allocator,
	VkShaderStateRuntimeInfo& parentRuntime,
	VkShaderStateSetupInfo& parentSetup,
	const Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& buffersByGroup,
	const IndexT group
);

/// sets up variables
void VkShaderStateSetupConstants(
	const Ids::Id24 id,
	AnyFX::ShaderEffect* effect,
	VkShaderStateRuntimeInfo& runtime,
	VkShaderStateSetupInfo& setup,
	VkShaderConstantAllocator& varAllocator,
	const Util::Array<IndexT>& groups);

/// setup uniform buffers for shader state
void VkShaderStateSetupConstantBuffers(
	const Ids::Id24 id,
	AnyFX::ShaderEffect* effect,
	VkShaderStateRuntimeInfo& runtime,
	VkShaderStateSetupInfo& setup,
	VkShaderConstantAllocator& varAllocator,
	Util::Array<VkWriteDescriptorSet>& setWrites,
	const Util::Array<IndexT>& groups,
	const Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers);

/// discard state
void VkShaderStateDiscard(
	const Ids::Id24 id,
	VkShaderStateRuntimeInfo& runtime,
	VkShaderStateSetupInfo& setup,
	VkShaderConstantAllocator& varAllocator
);

/// commit changes before rendering
void VkShaderStateCommit(Ids::Id24 currentProgram, Util::Array<VkWriteDescriptorSet>& writes, VkShaderStateRuntimeInfo& stateInfo);

/// use this if some system want to allocate and use their own descriptor sets
void VkShaderStateSetDescriptorSet(VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, const VkDescriptorSet& set, const IndexT slot);
/// create array of offsets
void VkShaderStateCreateOffsetArray(VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, Util::Array<uint32_t>& outOffsets, const IndexT group);
/// get index in offset array based on binding
VkShaderStateBufferMapping VkShaderStateGetBufferMapping(VkShaderStateRuntimeInfo& runtime, VkShaderStateSetupInfo& setup, const IndexT& group, const IndexT& binding);

/// apply a derivative state to its buffers
void VkShaderStateDerivativeStateApply(const VkDerivativeShaderStateRuntimeInfo& info);
/// commit derivative state to the graphics context
void VkShaderStateDerivativeStateCommit(const VkDerivativeShaderStateRuntimeInfo& info);
/// reset derivative state
void VkShaderStateDerivativeStateReset(VkDerivativeShaderStateRuntimeInfo& info);
/// tell device to update a set of descriptor sets
void VkShaderStateUpdateDescriptorSets(VkDevice dev, Util::Array<VkWriteDescriptorSet>& writes, bool& dirty);

/// set the descriptor set
void VkShaderStateSetDescriptorSet(CoreGraphics::ShaderStateId id, VkDescriptorSet set, const IndexT group);
/// set the state to be applied shared (as in shared across several shaders)
void VkShaderStateSetShared(CoreGraphics::ShaderStateId id, bool b);
/// add a descriptor set write to be executed the next time this state is used
void VkShaderStateAddDescriptorSetWrite(CoreGraphics::ShaderStateId id, VkWriteDescriptorSet write);

} // namespace Vulkan