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
#include "coregraphics/coregraphics.h"

namespace CoreGraphics
{
class ConstantBuffer;
}
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
	void BindShader(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
	/// bind shader using only a concatenated shader-program id
	void BindShader(const CoreGraphics::ShaderProgramId shaderProgramId);
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

	/// get number of variables for shader
	uint32_t GetVariableCount(const CoreGraphics::ShaderId id);
	/// get number of variable blocks
	uint32_t GetVariableBlockCount(const CoreGraphics::ShaderId id);
	/// get shader variable id
	CoreGraphics::ShaderVariableId GetShaderVariable(const CoreGraphics::ShaderStateId state, const Util::StringAtom& name);
	/// get shader variable by index
	CoreGraphics::ShaderVariableId GetShaderVariable(const CoreGraphics::ShaderStateId state, const IndexT index);
	/// set shader variable
	template <class TYPE> void SetShaderVariable(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE& value);
	/// set shader variable array
	template <class TYPE> void SetShaderVariableArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const TYPE* value, uint32_t count);
	/// set variable as texture
	void SetShaderVariableTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Resources::ResourceId texture);
	/// set variable as texture
	void SetShaderVariableConstantBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Resources::ResourceId buffer);
	/// set variable as texture
	void SetShaderVariableReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Resources::ResourceId image);
	/// set variable as texture
	void SetShaderVariableReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex);
	/// set variable as texture
	void SetShaderVariableReadWriteBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf);
private:
	/// get shader program
	AnyFX::VkProgram* GetProgram(const Ids::Id24 shaderId, const Ids::Id24 programId);
	/// load shader
	LoadStatus Load(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
	/// unload shader
	void Unload(const Ids::Id24 id);

	typedef Util::Dictionary<uint32_t, Util::Array<VkDescriptorSetLayoutBinding>> SetBinding;
	typedef Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ConstantBuffer>> UniformBufferMap;
	typedef Util::Dictionary<uint32_t, Util::Array<Ptr<CoreGraphics::ConstantBuffer>>> UniformBufferGroupMap;
	typedef Util::Dictionary<CoreGraphics::ShaderFeature::Mask, Ids::Id64> ProgramMap;

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

	struct SetupInfo
	{
		Resources::ResourceName name;
		CoreGraphics::ShaderIdentifier::Code id;
		VkPipelineLayout pipelineLayout;
		VkPushConstantRange constantRangeLayout;
		Util::Array<VkSampler> immutableSamplers;
		Util::FixedArray<VkDescriptorSetLayout> descriptorSetLayouts;
		SetBinding setBindings;
		Util::FixedArray<VkDescriptorSet> sets;
		UniformBufferMap uniformBufferMap;
		UniformBufferGroupMap uniformBufferGroupMap;
	};

	struct RuntimeInfo
	{
		CoreGraphics::ShaderFeature::Mask activeMask;
		Ids::Id64 activeShaderProgram;
		ProgramMap programMap;
	};

	/// this member allocates shaders
	Ids::IdAllocator<
		AnyFX::ShaderEffect*,						//0 effect
		SetupInfo,									//1 setup immutable values
		RuntimeInfo,								//2 runtime values
		VkShaderProgram::ProgramAllocator,			//3 variations
		VkShaderState::ShaderStateAllocator			//4 the shader states, sorted by shader
	> shaderAlloc;
	__ImplementResourceAllocator(shaderAlloc);	

	//__ResourceAllocator(VkShader);
	Ids::Id64 activeShaderProgram;
	CoreGraphics::ShaderFeature::Mask activeMask;
	Util::Dictionary<Ids::Id24, Ids::Id32> sharedStateMap;
};

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariable(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const int& value)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetInt(alloc.Get<0>(var), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariableArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const int* value, uint32_t count)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetIntArray(alloc.Get<0>(var), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariable(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const float& value)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetFloat(alloc.Get<0>(var), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariableArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const float* value, uint32_t count)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetFloatArray(alloc.Get<0>(var), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariable(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float2& value)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetFloat2(alloc.Get<0>(var), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariableArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float2* value, uint32_t count)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetFloat2Array(alloc.Get<0>(var), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariable(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float4& value)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetFloat4(alloc.Get<0>(var), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariableArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::float4* value, uint32_t count)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetFloat4Array(alloc.Get<0>(var), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariable(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::matrix44& value)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetMatrix(alloc.Get<0>(var), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariableArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Math::matrix44* value, uint32_t count)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetMatrixArray(alloc.Get<0>(var), value, count);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariable(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const bool& value)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetBool(alloc.Get<0>(var), value);
}

//------------------------------------------------------------------------------
/**
*/
template <> void
VkShaderPool::SetShaderVariableArray(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const bool* value, uint32_t count)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
	VkShaderVariable::SetBoolArray(alloc.Get<0>(var), value, count);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::SetShaderVariableTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Resources::ResourceId texture)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderVariable::ShaderVariableAllocator& alloc = this->shaderAlloc.Get<4>(shaderId).Get<3>(stateId);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::SetShaderVariableConstantBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Resources::ResourceId buffer)
{

}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::SetShaderVariableReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Resources::ResourceId image)
{
	VkTexture::textureAllocator.EnterGet();
	VkTexture::RuntimeInfo& info = VkTexture::textureAllocator.Get<0>(var);
	VkTexture::textureAllocator.LeaveGet();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::SetShaderVariableReadWriteTexture(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderState::ShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(shaderId);
	VkShaderVariable::ShaderVariableAllocator& varAlloc = stateAlloc.Get<3>(stateId);
	VkShaderVariable::SetShaderReadWriteTexture(varAlloc.Get<1>(var), stateAlloc.Get<4>(stateId), tex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderPool::SetShaderVariableReadWriteBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ShaderStateId state, const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	VkShaderState::ShaderStateAllocator& stateAlloc = this->shaderAlloc.Get<4>(shaderId);
	VkShaderVariable::ShaderVariableAllocator& varAlloc = stateAlloc.Get<3>(stateId);
	VkShaderVariable::SetShaderReadWriteBuffer(varAlloc.Get<1>(var), stateAlloc.Get<4>(stateId), buf);
}

} // namespace Vulkan