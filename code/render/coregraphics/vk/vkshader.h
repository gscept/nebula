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
#include "coregraphics/sampler.h"
#include "coregraphics/resourcetable.h"

namespace Vulkan
{

/// create descriptor set layout
void VkShaderSetup(
	VkDevice dev,
	const VkPhysicalDeviceProperties props,
	AnyFX::ShaderEffect* effect,
	VkPushConstantRange& constantRange,
	Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
	Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
	Util::Dictionary<uint32_t, uint32_t>& setLayoutMap,
	CoreGraphics::ResourcePipelineId& pipelineLayout,
	Util::FixedArray<CoreGraphics::ResourceTableId>& sets,
	Util::Dictionary<Util::StringAtom, uint32_t>& resourceSlotMap,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& sharedBuffers,
	Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& sharedBuffersByGroup
);
/// cleanup shader
void VkShaderCleanup(
	VkDevice dev,
	Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
	Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
	CoreGraphics::ResourcePipelineId& pipelineLayout
);

/// returns the binding of a resource variable using shader state
uint32_t VkShaderGetVkShaderVariableBinding(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderConstantId var);

/// create descriptor layout signature
static Util::String VkShaderCreateSignature(const VkDescriptorSetLayoutBinding& bind);



extern Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> VkShaderLayoutCache;
extern Util::Dictionary<Util::StringAtom, VkPipelineLayout> VkShaderPipelineCache;
extern Util::Dictionary<Util::StringAtom, VkDescriptorSet> VkShaderDescriptorSetCache;
} // namespace Vulkan