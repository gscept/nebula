#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader effect (using AnyFX) in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/shaderbase.h"
#include "lowlevel/afxapi.h"
#include "util/set.h"

#define AMD_DESC_SETS 1
namespace CoreGraphics
{
	class ConstantBuffer;
}

namespace Vulkan
{
class VkShader : public Base::ShaderBase
{
	__DeclareClass(VkShader);
public:
	/// constructor
	VkShader();
	/// destructor
	virtual ~VkShader();

	/// unload the resource, or cancel the pending load
	virtual void Unload();
	/// returns effect
	AnyFX::ShaderEffect* GetVkEffect() const;

	/// create descriptor set layout
	void Setup(AnyFX::ShaderEffect* effect);
	/// get variable offset (within its constant buffer) by name
	static const uint32_t& GetVariableOffset(const Util::String& name);
	/// get variable offset by index
	static const uint32_t& GetVariableOffset(const IndexT& i);
	/// returns variable offset index
	static IndexT FindVariableOffset(const Util::String& name);
	/// returns constant offset table using varblock signature
	static const Util::Dictionary<Util::StringAtom, uint32_t>& GetConstantOffsetTable(const Util::StringAtom& signature);

	/// get layout for specific descriptor set
	const VkDescriptorSetLayout& GetDescriptorLayout(const IndexT group);
	/// get pipeline layout
	const VkPipelineLayout& GetPipelineLayout() const;

	/// reloads a shader from file
	void Reload();

private:
	friend class VkShaderPool;
	friend class VkShaderState;

	/// create descriptor layout signature
	Util::String CreateSignature(const VkDescriptorSetLayoutBinding& bind);

	/// cleans up the shader
	void Cleanup();

	/// called by ogl4 shader server when ogl4 device is lost
	void OnLostDevice();
	/// called by ogl4 shader server when ogl4 device is reset
	void OnResetDevice();

	AnyFX::ShaderEffect* vkEffect;
	VkPipelineLayout pipelineLayout;

	Util::Array<VkSampler> immutableSamplers;
	VkPushConstantRange constantRange;
	Util::FixedArray<VkDescriptorSetLayout> setLayouts;
	Util::Dictionary<IndexT, Util::Array<VkDescriptorSetLayoutBinding>> setBindings;
#if !AMD_DESC_SETS
	Util::Dictionary<IndexT, IndexT> setToIndexMap;
#endif

	Util::FixedArray<VkDescriptorSet> sets;
	Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ConstantBuffer>> buffers;
	Util::Dictionary<uint32_t, Util::Array<Ptr<CoreGraphics::ConstantBuffer>>> buffersByGroup;

	static Util::Dictionary<Util::StringAtom, VkDescriptorSetLayout> LayoutCache;
	static Util::Dictionary<Util::StringAtom, VkPipelineLayout> ShaderPipelineCache;
	static Util::Dictionary<Util::StringAtom, VkDescriptorSet> DescriptorSetCache;

	Util::Set<Util::String> activeBlockNames;
};

//------------------------------------------------------------------------------
/**
*/
inline AnyFX::ShaderEffect*
VkShader::GetVkEffect() const
{
	return this->vkEffect;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkDescriptorSetLayout&
VkShader::GetDescriptorLayout(const IndexT group)
{
	return this->setLayouts[group];
}

//------------------------------------------------------------------------------
/**
*/
inline const VkPipelineLayout&
VkShader::GetPipelineLayout() const
{
	return this->pipelineLayout;
}

} // namespace Vulkan