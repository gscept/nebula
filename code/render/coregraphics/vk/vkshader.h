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
class VkShader
{
public:
	/// create descriptor set layout
	static void Setup(
		VkDevice dev,
		const VkPhysicalDeviceProperties props,
		AnyFX::ShaderEffect* effect, 
		VkPushConstantRange& constantRange, 
		Util::Dictionary<uint32_t, Util::Array<VkDescriptorSetLayoutBinding>>& setBindings,
		Util::Array<VkSampler>& immutableSamplers,
		Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
		VkPipelineLayout& pipelineLayout,
		Util::FixedArray<VkDescriptorSet>& sets,
		Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& buffers,
		Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>>& buffersByGroup
		);
	/// cleanup shader
	static void Cleanup(
		VkDevice dev,
		Util::Array<VkSampler>& immutableSamplers,
		Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
		VkPipelineLayout& pipelineLayout
	);
	/// get variable offset (within its constant buffer) by name
	static const uint32_t& GetVariableOffset(const Util::String& name);
	/// get variable offset by index
	static const uint32_t& GetVariableOffset(const IndexT& i);
	/// returns variable offset index
	static IndexT FindVariableOffset(const Util::String& name);
	/// returns constant offset table using varblock signature
	static const Util::Dictionary<Util::StringAtom, uint32_t>& GetConstantOffsetTable(const Util::StringAtom& signature);

	/// returns the binding of a resource object in a shader
	static uint32_t ShaderGetVkVariableBinding(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderVariableId var);

private:
	friend class VkShaderPool;
	friend class VkShaderState;

	/// create descriptor layout signature
	static Util::String CreateSignature(const VkDescriptorSetLayoutBinding& bind);

	static Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> LayoutCache;
	static Util::Dictionary<Util::StringAtom, VkPipelineLayout> ShaderPipelineCache;
	static Util::Dictionary<Util::StringAtom, VkDescriptorSet> DescriptorSetCache;
};


} // namespace Vulkan