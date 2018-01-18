#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader loader from stream into a Vulkan shader.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "util/dictionary.h"
#include "coregraphics/shaderfeature.h"
#include "lowlevel/vk/vkprogram.h"
#include "coregraphics/shader.h"
#include "vulkan/vulkan.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include "coregraphics/shaderidentifier.h"
#include "vkshaderprogram.h"
#include "vkshaderstate.h"
#include "vktexture.h"
#include "vkshadervariable.h"
#include "vkshader.h"


namespace Vulkan
{

class VkShaderPool : public Resources::ResourceStreamPool
{
	__DeclareClass(VkShaderPool);

public:

	/// constructor
	VkShaderPool();
	/// destructor
	virtual ~VkShaderPool();

	/// bind shader (and variation within shader)
	void ShaderBind(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
	/// bind shader using only a concatenated shader-program id
	void ShaderBind(const CoreGraphics::ShaderProgramId shaderProgramId);
	/// get shader-program id, which can be used to directly access a program in a shader
	CoreGraphics::ShaderProgramId GetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
	/// create a new shader state
	CoreGraphics::ShaderStateId CreateState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups, bool createUniqueSet);
	/// create a shared state, might return a new StateId, or a previously allocated one
	CoreGraphics::ShaderStateId CreateSharedState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups);
	/// destroy a shader state
	void DestroyState(const CoreGraphics::ShaderStateId state);
	/// apply state
	void ApplyState(const CoreGraphics::ShaderStateId state);

	/// create derivative
	CoreGraphics::DerivativeStateId CreateDerivativeState(const CoreGraphics::ShaderStateId id, const IndexT group);
	/// destroy derivative
	void DestroyDerivativeState(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId& deriv);
	/// apply derivative state
	void DerivativeStateApply(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId& deriv);
	/// commit derivative state
	void DerivativeStateCommit(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId& deriv);
	/// reset derivative state
	void DerivativeStateReset(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId& deriv);

	/// get number of variables for shader
	const SizeT GetVariableCount(const CoreGraphics::ShaderId id) const;
	/// get number of variable blocks
	const SizeT GetVariableBlockCount(const CoreGraphics::ShaderId id) const;
	/// get shader variable id
	const CoreGraphics::ShaderVariableId ShaderStateGetVariable(const CoreGraphics::ShaderStateId state, const Util::StringAtom& name) const;
	/// get shader variable by index
	const CoreGraphics::ShaderVariableId ShaderStateGetVariable(const CoreGraphics::ShaderStateId state, const IndexT index) const;
	/// set shader variable
	template <class TYPE> void ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE& value);
	/// set shader variable array
	template <class TYPE> void ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE* value, uint32_t count);
	/// set variable as texture
	void ShaderVariableSetTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId texture);
	/// set variable as texture
	void ShaderVariableSetConstantBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ConstantBufferId buffer);
	/// set variable as texture
	void ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId image);
	/// set variable as texture
	void ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex);
	/// set variable as texture
	void ShaderVariableSetReadWriteBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWBufferId buf);

	/// get pipeline layout for shader
	const VkPipelineLayout GetPipelineLayout(const CoreGraphics::ShaderId id) const;
	/// get descriptor set layout for shader and group
	const VkDescriptorSetLayout GetDescriptorSetLayout(const CoreGraphics::ShaderId id, const IndexT group) const;

	/// get state allocator for shader
	VkShaderStateAllocator& GetStateAllocator(const CoreGraphics::ShaderStateId id);

private:
	friend class VkVertexSignaturePool;
	friend class VkPipelineDatabase;
	friend const CoreGraphics::ConstantBufferId CoreGraphics::CreateConstantBuffer(const CoreGraphics::ConstantBufferCreateInfo& info);
	friend uint32_t	VkShader::ShaderGetVkVariableBinding(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderVariableId var);

	/// get shader program
	AnyFX::VkProgram* GetProgram(const CoreGraphics::ShaderProgramId shaderProgramId);
	/// load shader
	LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) override;
	/// unload shader
	void Unload(const Ids::Id24 id) override;

	typedef Util::Dictionary<uint32_t, Util::Array<VkDescriptorSetLayoutBinding>> SetBinding;
	typedef Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId> UniformBufferMap;
	typedef Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>> UniformBufferGroupMap;
	typedef Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId> ProgramMap;

#pragma pack(push, 16)
	struct DescriptorSetBinding
	{
		VkDescriptorSet set;
		VkPipelineLayout layout;
		IndexT slot;
	};

	struct BufferMapping
	{
		uint32_t index;
		uint32_t offset;
	};
#pragma pack(pop)

	struct VkShaderSetupInfo
	{
		Resources::ResourceName name;
		CoreGraphics::ShaderIdentifier::Code id;
		UniformBufferMap uniformBufferMap;
		UniformBufferGroupMap uniformBufferGroupMap;

		VkPipelineLayout pipelineLayout;
		VkPushConstantRange constantRangeLayout;
		Util::Array<VkSampler> immutableSamplers;
		Util::FixedArray<VkDescriptorSetLayout> descriptorSetLayouts;
		SetBinding setBindings;
		Util::FixedArray<VkDescriptorSet> sets;
	};

	struct VkShaderRuntimeInfo
	{
		CoreGraphics::ShaderFeature::Mask activeMask;
		CoreGraphics::ShaderProgramId activeShaderProgram;
		ProgramMap programMap;
	};

	/// this member allocates shaders
	Ids::IdAllocator<
		AnyFX::ShaderEffect*,						//0 effect
		VkShaderSetupInfo,							//1 setup immutable values
		VkShaderRuntimeInfo,						//2 runtime values
		VkShaderProgram::ProgramAllocator,			//3 variations
		VkShaderStateAllocator						//4 the shader states, sorted by shader
	> shaderAlloc;
	__ImplementResourceAllocator(shaderAlloc);	

	//__ResourceAllocator(VkShader);
	CoreGraphics::ShaderProgramId activeShaderProgram;
	CoreGraphics::ShaderFeature::Mask activeMask;
	Util::Dictionary<Ids::Id24, Ids::Id32> sharedStateMap;
};

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const int& value)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetInt(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const int* value, uint32_t count)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetIntArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const float& value)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetFloat(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const float* value, uint32_t count)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetFloatArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float2& value)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetFloat2(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float2* value, uint32_t count)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetFloat2Array(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float4& value)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetFloat4(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float4* value, uint32_t count)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetFloat4Array(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::matrix44& value)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetMatrix(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::matrix44* value, uint32_t count)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetMatrixArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSet(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const bool& value)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetBool(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::ShaderVariableSetArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const bool* value, uint32_t count)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	SetBoolArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderVariableSetTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId texture)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.id24);
	VkShaderVariableAllocator& alloc = stateAlloc.Get<3>(state.id32);
	SetTexture(alloc.Get<0>(var.id), alloc.Get<1>(var.id), stateAlloc.Get<4>(state.id24), texture);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderVariableSetConstantBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ConstantBufferId buffer)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.id24);
	VkShaderVariableAllocator& alloc = stateAlloc.Get<3>(state.id32);
	SetConstantBuffer(alloc.Get<1>(var.id), stateAlloc.Get<4>(state.id24), buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId image)
{
	VkShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(state.id24).Get<3>(state.id32);
	textureAllocator.EnterGet();
	VkTextureRuntimeInfo& info = textureAllocator.Get<0>(var);
	textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderVariableSetReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.id24);
	VkShaderVariableAllocator& varAlloc = stateAlloc.Get<3>(state.id32);
	SetShaderReadWriteTexture(varAlloc.Get<1>(var), stateAlloc.Get<4>(state.id24), tex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderVariableSetReadWriteBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWBufferId buf)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.id24);
	VkShaderVariableAllocator& varAlloc = stateAlloc.Get<3>(state.id32);
	SetShaderReadWriteBuffer(varAlloc.Get<1>(var), stateAlloc.Get<4>(state.id24), buf);
}

//------------------------------------------------------------------------------
/**
*/
inline const VkPipelineLayout
VkShaderPool::GetPipelineLayout(const CoreGraphics::ShaderId id) const
{
	const VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<1>(id.id24);
	return setupInfo.pipelineLayout;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkDescriptorSetLayout
VkShaderPool::GetDescriptorSetLayout(const CoreGraphics::ShaderId id, const IndexT group) const
{
	const VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<1>(id.id24);
	return setupInfo.descriptorSetLayouts[group];
}

//------------------------------------------------------------------------------
/**
*/
inline VkShaderStateAllocator&
VkShaderPool::GetStateAllocator(const CoreGraphics::ShaderStateId id)
{
	return this->shaderAlloc.Get<4>(id.id24);
}

} // namespace Vulkan