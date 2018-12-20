#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader effect (using AnyFX) in Vulkan.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
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
	const Util::StringAtom& name,
	const VkPhysicalDeviceProperties props,
	AnyFX::ShaderEffect* effect,
	Util::FixedArray<CoreGraphics::ResourcePipelinePushConstantRange>& constantRange,
	Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
	Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
	Util::Dictionary<uint32_t, uint32_t>& setLayoutMap,
	CoreGraphics::ResourcePipelineId& pipelineLayout,
	Util::Dictionary<Util::StringAtom, uint32_t>& resourceSlotMapping,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBinding>& constantBindings
);
/// cleanup shader
void VkShaderCleanup(
	VkDevice dev,
	Util::Array<CoreGraphics::SamplerId>& immutableSamplers,
	Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>>& setLayouts,
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
	CoreGraphics::ResourcePipelineId& pipelineLayout
);

/// create descriptor layout signature
static Util::String VkShaderCreateSignature(const VkDescriptorSetLayoutBinding& bind);

extern Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> VkShaderLayoutCache;
extern Util::Dictionary<Util::StringAtom, VkPipelineLayout> VkShaderPipelineCache;
extern Util::Dictionary<Util::StringAtom, VkDescriptorSet> VkShaderDescriptorSetCache;
} // namespace Vulkan