#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader effect (using AnyFX) in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "lowlevel/afxapi.h"
#include "util/set.h"
#include "coregraphics/shader.h"

namespace Vulkan
{

/// create descriptor set layout
void VkShaderSetup(
	VkDevice dev,
	const VkPhysicalDeviceProperties props,
	AnyFX::ShaderEffect* effect,
	VkPushConstantRange& constantRange,
	Util::Dictionary<uint32_t, Util::Array<VkDescriptorSetLayoutBinding>>& setBindings,
	Util::Array<VkSampler>& immutableSamplers,
	Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
	VkPipelineLayout& pipelineLayout,
	Util::FixedArray<VkDescriptorSet>& sets,
	Util::FixedArray<VkDescriptorPool>& setPools,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
	Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& buffersByGroup
);
/// cleanup shader
void VkShaderCleanup(
	VkDevice dev,
	Util::Array<VkSampler>& immutableSamplers,
	Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
	VkPipelineLayout& pipelineLayout
);
/// get variable offset (within its constant buffer) by name
const uint32_t& VkShaderGetVariableOffset(const Util::String& name);
/// get variable offset by index
const uint32_t& VkShaderGetVariableOffset(const IndexT& i);
/// returns variable offset index
IndexT VkShaderFindVariableOffset(const Util::String& name);
/// returns constant offset table using varblock signature
const Util::Dictionary<Util::StringAtom, uint32_t>& VkShaderGetConstantOffsetTable(const Util::StringAtom& signature);

/// returns the binding of a resource variable using shader state
uint32_t VkShaderGetVkShaderVariableBinding(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderVariableId var);
/// return the descriptor set which a resource variable is bound to
VkDescriptorSet VkShaderGetVkShaderVariableDescriptorSet(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderVariableId var);

/// create descriptor layout signature
static Util::String VkShaderCreateSignature(const VkDescriptorSetLayoutBinding& bind);

extern Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> VkShaderLayoutCache;
extern Util::Dictionary<Util::StringAtom, VkPipelineLayout> VkShaderPipelineCache;
extern Util::Dictionary<Util::StringAtom, VkDescriptorSet> VkShaderDescriptorSetCache;
} // namespace Vulkan