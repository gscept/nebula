#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader loader from stream into a Vulkan shader.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/config.h"
#include "resources/resourcestreampool.h"
#include "util/dictionary.h"
#include "coregraphics/shaderfeature.h"
#include "lowlevel/vk/vkprogram.h"
#include "coregraphics/shader.h"
#include "vulkan/vulkan.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/shaderidentifier.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/resourcetable.h"
#include "vkshaderprogram.h"
#include "vkshaderstate.h"
#include "vktexture.h"
#include "vkshaderconstant.h"
#include "vkshader.h"

namespace CoreGraphics
{
	void SetShaderState(const CoreGraphics::ShaderStateId& state);
	void SetShaderProgram(const CoreGraphics::ShaderProgramId& pro);
	void SetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
}

namespace Vulkan
{

class VkShaderPool : public Resources::ResourceStreamPool
{
	__DeclareClass(VkShaderPool);

public:

	typedef Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId> ProgramMap;
	struct VkShaderRuntimeInfo
	{
		CoreGraphics::ShaderFeature::Mask activeMask;
		CoreGraphics::ShaderProgramId activeShaderProgram;
		ProgramMap programMap;
	};

	/// constructor
	VkShaderPool();
	/// destructor
	virtual ~VkShaderPool();

	/// get shader-program id, which can be used to directly access a program in a shader
	CoreGraphics::ShaderProgramId GetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
	/// create a new shader state
	CoreGraphics::ShaderStateId CreateState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups, bool createUniqueSet);
	/// create a shared state, might return a new StateId, or a previously allocated one
	CoreGraphics::ShaderStateId CreateSlicedState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups);
	/// destroy a shader state
	void DestroyState(const CoreGraphics::ShaderStateId state);
	/// commit changes done to state
	void CommitState(const CoreGraphics::ShaderStateId state);
	/// get number of used states
	SizeT GetNumActiveStates(const CoreGraphics::ShaderId shaderId);
	/// get the resource table layout
	CoreGraphics::ResourceTableLayoutId GetResourceTableLayout(const CoreGraphics::ShaderId id, const IndexT group);
	/// get the pipeline layout
	CoreGraphics::ResourcePipelineId GetResourcePipeline(const CoreGraphics::ShaderId id);

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
	const SizeT GetConstantCount(const CoreGraphics::ShaderId id) const;
	/// get type of constant by index
	const CoreGraphics::ShaderConstantType GetConstantType(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get name of constant by index
	const Util::StringAtom GetConstantName(const CoreGraphics::ShaderId id, const IndexT i) const;

	/// get number of constant blocks
	const SizeT GetConstantBufferCount(const CoreGraphics::ShaderId id) const;
	/// get size of constant buffer
	const SizeT GetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get name of constnat buffer
	const Util::StringAtom GetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get slot of shader resource
	const IndexT GetResourceSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const;

	/// get list all mask-program pairs
	const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>& GetPrograms(const CoreGraphics::ShaderId id);
	/// get name of program
	Util::String GetProgramName(CoreGraphics::ShaderProgramId id);

	/// get shader constant id
	const CoreGraphics::ShaderConstantId ShaderStateGetConstant(const CoreGraphics::ShaderStateId state, const Util::StringAtom& name) const;
	/// get shader constant by index
	const CoreGraphics::ShaderConstantId ShaderStateGetConstant(const CoreGraphics::ShaderStateId state, const IndexT index) const;
	/// get the constant type
	const CoreGraphics::ShaderConstantType ShaderConstantGetType(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state) const;

	/// set shader constant
	template <class TYPE> void ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const TYPE& value);
	/// set shader constant array
	template <class TYPE> void ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const TYPE* value, uint32_t count);
	/// set constant as render texture in texture slot
	void ShaderResourceSetRenderTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::RenderTextureId texture);
	/// set constant as texture
	void ShaderResourceSetTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId texture);
	/// set constant as constant buffer
	void ShaderResourceSetConstantBuffer(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ConstantBufferId buffer);
	/// set constant as texture
	void ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId image);
	/// set constant as texture
	void ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex);
	/// set constant as texture
	void ShaderResourceSetReadWriteBuffer(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWBufferId buf);

	/// get state allocator for shader
	VkShaderStateAllocator& GetStateAllocator(const CoreGraphics::ShaderStateId id);

private:
	friend class VkVertexSignaturePool;
	friend class VkPipelineDatabase;
	friend const CoreGraphics::ConstantBufferId CoreGraphics::CreateConstantBuffer(const CoreGraphics::ConstantBufferCreateInfo& info);
	friend uint32_t	VkShaderGetVkShaderVariableBinding(const CoreGraphics::ShaderStateId shader, const CoreGraphics::ShaderConstantId var);

	friend void	CoreGraphics::SetShaderState(const CoreGraphics::ShaderStateId& state);
	friend void CoreGraphics::SetShaderProgram(const CoreGraphics::ShaderProgramId& pro);
	friend void CoreGraphics::SetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);

	/// get shader program
	AnyFX::VkProgram* GetProgram(const CoreGraphics::ShaderProgramId shaderProgramId);
	/// load shader
	LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) override;
	/// unload shader
	void Unload(const Resources::ResourceId id) override;

	typedef Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId> UniformBufferMap;
	typedef Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>> UniformBufferGroupMap;

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
		VkDevice dev;
		Resources::ResourceName name;
		CoreGraphics::ShaderIdentifier::Code id;
		UniformBufferMap uniformBufferMap;				// uniform buffers shared by all shader states
		UniformBufferGroupMap uniformBufferGroupMap;	// same as above but grouped

		CoreGraphics::ResourcePipelineId pipelineLayout;
		Util::FixedArray<CoreGraphics::ResourceTableId> tables;
		VkPushConstantRange constantRangeLayout;
		Util::Array<CoreGraphics::SamplerId> immutableSamplers;
		Util::Dictionary<Util::StringAtom, uint32_t> resourceIndexMap;
		Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>> descriptorSetLayouts;
	};

	/// this member allocates shaders
	Ids::IdAllocator<
		AnyFX::ShaderEffect*,						//0 effect
		VkShaderSetupInfo,							//1 setup immutable values
		VkShaderRuntimeInfo,						//2 runtime values
		VkShaderProgramAllocator,					//3 variations
		VkShaderStateAllocator						//4 the shader states, sorted by shader
	> shaderAlloc;
	__ImplementResourceAllocatorTyped(shaderAlloc, ShaderIdType);	

	//__ResourceAllocator(VkShader);
	CoreGraphics::ShaderProgramId activeShaderProgram;
	CoreGraphics::ShaderFeature::Mask activeMask;
	Util::Dictionary<Ids::Id24, Ids::Id32> slicedStateMap;
};

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const int& value)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetInt(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const int* value, uint32_t count)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetIntArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const float& value)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetFloat(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const float* value, uint32_t count)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetFloatArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const Math::float2& value)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetFloat2(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const Math::float2* value, uint32_t count)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetFloat2Array(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const Math::float4& value)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetFloat4(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const Math::float4* value, uint32_t count)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetFloat4Array(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const Math::matrix44& value)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetMatrix(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const Math::matrix44* value, uint32_t count)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetMatrixArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSet(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const bool& value)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetBool(alloc.Get<0>(var.id), value);
}

//------------------------------------------------------------------------------
/**
*/
template<> inline void
VkShaderPool::ShaderConstantSetArray(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const bool* value, uint32_t count)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	SetBoolArray(alloc.Get<0>(var.id), value, count);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderResourceSetRenderTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::RenderTextureId texture)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.shaderId);
	VkShaderConstantAllocator& alloc = stateAlloc.Get<3>(state.stateId);
	SetRenderTexture(alloc.Get<0>(var.id), alloc.Get<1>(var.id), texture);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderResourceSetTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId texture)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.shaderId);
	VkShaderConstantAllocator& alloc = stateAlloc.Get<3>(state.stateId);
	SetTexture(alloc.Get<0>(var.id), alloc.Get<1>(var.id), texture);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderResourceSetConstantBuffer(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ConstantBufferId buffer)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.shaderId);
	VkShaderConstantAllocator& alloc = stateAlloc.Get<3>(state.stateId);
	SetConstantBuffer(alloc.Get<1>(var.id), buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::TextureId image)
{
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	textureAllocator.EnterGet();
	VkTextureRuntimeInfo& info = textureAllocator.Get<0>(var.id);
	textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderResourceSetReadWriteTexture(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWTextureId tex)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.shaderId);
	VkShaderConstantAllocator& varAlloc = stateAlloc.Get<3>(state.stateId);
	SetShaderReadWriteTexture(varAlloc.Get<0>(var.id), varAlloc.Get<1>(var.id), tex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::ShaderResourceSetReadWriteBuffer(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state, const CoreGraphics::ShaderRWBufferId buf)
{
	VkShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(state.shaderId);
	VkShaderConstantAllocator& varAlloc = stateAlloc.Get<3>(state.stateId);
	SetShaderReadWriteBuffer(varAlloc.Get<1>(var.id), buf);
}

//------------------------------------------------------------------------------
/**
*/
inline VkShaderStateAllocator&
VkShaderPool::GetStateAllocator(const CoreGraphics::ShaderStateId id)
{
	return this->shaderAlloc.Get<4>(id.shaderId);
}

} // namespace Vulkan