//------------------------------------------------------------------------------
// vkstreamshaderloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderpool.h"
#include "vkshader.h"
#include "coregraphics/constantbuffer.h"
#include "effectfactory.h"
#include "coregraphics/config.h"
#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"

using namespace CoreGraphics;
using namespace IO;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderPool, 'VKSL', Resources::ResourceStreamPool);

//------------------------------------------------------------------------------
/**
*/

VkShaderPool::VkShaderPool() :
	shaderAlloc(0x00FFFFFF),
	activeShaderProgram(Ids::InvalidId64),
	activeMask(0) 
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/

VkShaderPool::~VkShaderPool()
{

}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkShaderPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	n_assert(this->GetState(id) == Resources::Resource::Pending);

	void* srcData = stream->Map();
	uint srcDataSize = stream->GetSize();

	// load effect from memory
	AnyFX::ShaderEffect* effect = AnyFX::EffectFactory::Instance()->CreateShaderEffectFromMemory(srcData, srcDataSize);

	// catch any potential error coming from AnyFX
	if (!effect)
	{
		n_error("VkStreamShaderLoader::SetupResourceFromStream(): failed to load shader '%s'!",
			this->GetName(id).Value());
		return ResourcePool::Failed;
	}

	VkShaderSetupInfo& setupInfo = this->shaderAlloc.Get<1>(id.allocId);
	VkShaderRuntimeInfo& runtimeInfo = this->shaderAlloc.Get<2>(id.allocId);

	this->shaderAlloc.Get<0>(id.allocId) = effect;
	setupInfo.id = ShaderIdentifier::FromName(this->GetName(id));
	setupInfo.name = this->GetName(id);
	setupInfo.dev = Vulkan::GetCurrentDevice();

	// the setup code is massive, so just let it be in VkShader...
	VkShaderSetup(
		setupInfo.dev,
		Vulkan::GetCurrentProperties(),
		effect,
		setupInfo.constantRangeLayout,
		setupInfo.immutableSamplers,
		setupInfo.descriptorSetLayouts,
		setupInfo.pipelineLayout,
		setupInfo.tables,
		setupInfo.resourceIndexMap,
		setupInfo.uniformBufferMap,
		setupInfo.uniformBufferGroupMap
		);

	// setup shader variations
	const std::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
	for (uint i = 0; i < programs.size(); i++)
	{
		// get program object from shader subsystem
		VkShaderProgramAllocator& programAllocator = this->shaderAlloc.Get<3>(id.allocId);
		AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);

		// allocate new program object and set it up
		Ids::Id32 programId = programAllocator.AllocObject();
		VkShaderProgramSetup(programId, program, setupInfo.pipelineLayout, this->shaderAlloc.Get<3>(id.allocId));

		// make an ID which is the shader id and program id
		ShaderProgramId shaderProgramId;
		shaderProgramId.programId = programId;
		shaderProgramId.shaderId = id.allocId;
		shaderProgramId.shaderType = id.allocType;
		runtimeInfo.programMap.Add(programAllocator.Get<0>(programId).mask, shaderProgramId);
	}

	// set active variation
	runtimeInfo.activeMask = runtimeInfo.programMap.KeyAtIndex(0);
	runtimeInfo.activeShaderProgram = runtimeInfo.programMap.ValueAtIndex(0);

#if __NEBULA3_HTTP__
	//res->debugState = res->CreateState();
#endif
	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::Unload(const Resources::ResourceId res)
{
	VkShaderSetupInfo& setup = this->shaderAlloc.Get<1>(res.allocId);
	VkShaderProgramAllocator& programs = this->shaderAlloc.Get<3>(res.allocId);
	VkShaderRuntimeInfo& runtime = this->shaderAlloc.Get<2>(res.allocId);
	VkShaderCleanup(setup.dev, setup.immutableSamplers, setup.descriptorSetLayouts, setup.uniformBufferMap, setup.pipelineLayout);

	for (IndexT i = 0; i < runtime.programMap.Size(); i++)
	{
		VkShaderProgramSetupInfo& progSetup = programs.Get<0>(runtime.programMap.ValueAtIndex(i).programId);
		VkShaderProgramRuntimeInfo& progRuntime = programs.Get<2>(runtime.programMap.ValueAtIndex(i).programId);
		VkShaderProgramDiscard(progSetup, progRuntime, progRuntime.pipeline);
	}
	runtime.programMap.Clear();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderProgramId
VkShaderPool::GetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask)
{
	VkShaderRuntimeInfo& runtime = this->shaderAlloc.Get<2>(shaderId.allocId);
	return runtime.programMap[mask];
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderStateId
VkShaderPool::CreateState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups, bool createUniqueSet)
{
	// allocate a slice and create an id which is the shader id, and the state id
	VkShaderStateAllocator& stateAllocator = this->shaderAlloc.Get<4>(shader.allocId);
	Ids::Id32 stateId = stateAllocator.AllocObject();
	VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(shader.allocId);

	CoreGraphics::ShaderStateId ret = Ids::Id::MakeId24_8_24_8(shader.allocId, shader.allocType, stateId, ShaderStateIdType);

	VkShaderStateSetup(
		ret,
		this->shaderAlloc.Get<0>(shader.allocId),
		groups,
		stateAllocator,
		info.descriptorSetLayouts,
		info.uniformBufferMap,
		info.uniformBufferGroupMap,
		info.pipelineLayout
		);

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderStateId
VkShaderPool::CreateSlicedState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups)
{
	// allocate a slice and create an id which is the shader id, and the state id
	Ids::Id32 stateId = -1;
	if (this->slicedStateMap.Contains(shader.allocId)) stateId = this->slicedStateMap[shader.allocId];
	else
	{
		VkShaderStateAllocator& stateAllocator = this->shaderAlloc.Get<4>(shader.allocId);
		stateId = stateAllocator.AllocObject();
		VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(shader.allocId);
		VkShaderStateSetup(
			stateId,
			this->shaderAlloc.Get<0>(shader.allocId),
			groups,
			stateAllocator,
			info.descriptorSetLayouts,
			info.uniformBufferMap,
			info.uniformBufferGroupMap,
			info.pipelineLayout
		);
		this->slicedStateMap.Add(shader.allocId, stateId);
	}
	n_assert(stateId != -1);

	// the resource id is the shader id and the state id combined
	CoreGraphics::ShaderStateId ret = Ids::Id::MakeId24_8_24_8(shader.allocId, shader.allocType, stateId, ShaderStateIdType);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::DestroyState(const CoreGraphics::ShaderStateId state)
{
	VkShaderStateRuntimeInfo& runtime = this->shaderAlloc.Get<4>(state.shaderId).Get<1>(state.stateId);
	VkShaderStateSetupInfo& setup = this->shaderAlloc.Get<4>(state.shaderId).Get<2>(state.stateId);
	VkShaderConstantAllocator& alloc = this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId);
	VkShaderStateDiscard(runtime, setup, alloc);
	this->shaderAlloc.Get<4>(state.shaderId).DeallocObject(state.stateId);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::CommitState(const CoreGraphics::ShaderStateId state)
{
	VkShaderStateCommit(this->shaderAlloc.Get<4>(state.shaderId).Get<1>(state.stateId));
}

//------------------------------------------------------------------------------
/**
*/
SizeT
VkShaderPool::GetNumActiveStates(const CoreGraphics::ShaderId shaderId)
{
	return this->shaderAlloc.Get<4>(shaderId.allocId).GetNumUsed();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourceTableLayoutId
VkShaderPool::GetResourceTableLayout(const CoreGraphics::ShaderId id, const IndexT group)
{
	return std::get<1>(this->shaderAlloc.Get<1>(id.allocId).descriptorSetLayouts[group]);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ResourcePipelineId
VkShaderPool::GetResourcePipeline(const CoreGraphics::ShaderId id)
{
	return this->shaderAlloc.Get<1>(id.allocId).pipelineLayout;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::DerivativeStateId
VkShaderPool::CreateDerivativeState(const CoreGraphics::ShaderStateId id, const IndexT group)
{
	const AnyFX::ShaderEffect* effect = this->shaderAlloc.Get<0>(id.shaderId);
	const UniformBufferGroupMap& uniformBuffersByGroup = this->shaderAlloc.Get<1>(id.shaderId).uniformBufferGroupMap;
	VkDerivativeStateAllocator& derivAlloc = this->shaderAlloc.Get<4>(id.shaderId).Get<4>(id.stateId);
	VkShaderStateRuntimeInfo& parentRuntime = this->shaderAlloc.Get<4>(id.shaderId).Get<1>(id.stateId);
	VkShaderStateSetupInfo& parentSetup = this->shaderAlloc.Get<4>(id.shaderId).Get<2>(id.stateId);
	Ids::Id32 derivId = derivAlloc.AllocObject();

	VkDerivativeShaderStateRuntimeInfo& info = derivAlloc.Get<0>(derivId);
	VkShaderStateSetupDerivative(derivId, effect, derivAlloc, parentRuntime, parentSetup, uniformBuffersByGroup, group);

	DerivativeStateId ret;
	ret.id = derivId;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::DestroyDerivativeState(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId& deriv)
{
	VkDerivativeStateAllocator& derivAlloc = this->shaderAlloc.Get<4>(id.shaderId).Get<4>(id.stateId);
	VkDerivativeShaderStateRuntimeInfo& info = derivAlloc.Get<0>(deriv.id);
	derivAlloc.DeallocObject(deriv.id);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::DerivativeStateApply(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId& deriv)
{
	VkDerivativeShaderStateRuntimeInfo& info = this->shaderAlloc.Get<4>(id.shaderId).Get<4>(id.stateId).Get<0>(deriv.id);
	VkShaderStateDerivativeStateApply(info);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::DerivativeStateCommit(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId & deriv)
{
	VkDerivativeShaderStateRuntimeInfo& info = this->shaderAlloc.Get<4>(id.shaderId).Get<4>(id.stateId).Get<0>(deriv.id);
	VkShaderStateDerivativeStateCommit(info);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::DerivativeStateReset(const CoreGraphics::ShaderStateId id, const CoreGraphics::DerivativeStateId& deriv)
{
	VkDerivativeShaderStateRuntimeInfo& info = this->shaderAlloc.Get<4>(id.shaderId).Get<4>(id.stateId).Get<0>(deriv.id);
	VkShaderStateDerivativeStateReset(info);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantId
VkShaderPool::ShaderStateGetConstant(const CoreGraphics::ShaderStateId state, const Util::StringAtom& name) const
{
	return this->shaderAlloc.Get<4>(state.shaderId).Get<2>(state.stateId).variableMap[name];
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantId
VkShaderPool::ShaderStateGetConstant(const CoreGraphics::ShaderStateId state, const IndexT index) const
{
	return this->shaderAlloc.Get<4>(state.shaderId).Get<2>(state.stateId).variableMap.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderConstantType
VkShaderPool::ShaderConstantGetType(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ShaderStateId state) const
{
	return this->shaderAlloc.Get<4>(state.shaderId).Get<3>(state.stateId).Get<2>(var.id).type;
}

//------------------------------------------------------------------------------
/**
	Use direct resource ids, not the State, Shader or Variable type ids
*/
AnyFX::VkProgram*
VkShaderPool::GetProgram(const CoreGraphics::ShaderProgramId shaderProgramId)
{
	return this->shaderAlloc.Get<3>(shaderProgramId.shaderId).Get<1>(shaderProgramId.programId);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderPool::GetConstantCount(const CoreGraphics::ShaderId id) const
{
	return this->shaderAlloc.Get<0>(id.allocId)->GetNumVariables();
}

//------------------------------------------------------------------------------
/**
*/
const  CoreGraphics::ShaderConstantType
VkShaderPool::GetConstantType(const CoreGraphics::ShaderId id, const IndexT i) const
{
	AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.allocId)->GetVariable(i);
	switch (var->type)
	{
	case AnyFX::Double:
	case AnyFX::Float:
		return FloatVariableType;
	case AnyFX::Short:
	case AnyFX::Integer:
	case AnyFX::UInteger:
		return IntVariableType;
	case AnyFX::Bool:
		return BoolVariableType;
	case AnyFX::Float3:
	case AnyFX::Float4:
	case AnyFX::Double3:
	case AnyFX::Double4:
	case AnyFX::Integer3:
	case AnyFX::Integer4:
	case AnyFX::UInteger3:
	case AnyFX::UInteger4:
	case AnyFX::Short3:
	case AnyFX::Short4:
	case AnyFX::Bool3:
	case AnyFX::Bool4:
		return VectorVariableType;
	case AnyFX::Float2:
	case AnyFX::Double2:
	case AnyFX::Integer2:
	case AnyFX::UInteger2:
	case AnyFX::Short2:
	case AnyFX::Bool2:
		return Vector2VariableType;
	case AnyFX::Matrix2x2:
	case AnyFX::Matrix2x3:
	case AnyFX::Matrix2x4:
	case AnyFX::Matrix3x2:
	case AnyFX::Matrix3x3:
	case AnyFX::Matrix3x4:
	case AnyFX::Matrix4x2:
	case AnyFX::Matrix4x3:
	case AnyFX::Matrix4x4:
		return MatrixVariableType;
		break;
	case AnyFX::Image1D:
	case AnyFX::Image1DArray:
	case AnyFX::Image2D:
	case AnyFX::Image2DArray:
	case AnyFX::Image2DMS:
	case AnyFX::Image2DMSArray:
	case AnyFX::Image3D:
	case AnyFX::ImageCube:
	case AnyFX::ImageCubeArray:
		return ImageReadWriteVariableType;
	case AnyFX::Sampler1D:
	case AnyFX::Sampler1DArray:
	case AnyFX::Sampler2D:
	case AnyFX::Sampler2DArray:
	case AnyFX::Sampler2DMS:
	case AnyFX::Sampler2DMSArray:
	case AnyFX::Sampler3D:
	case AnyFX::SamplerCube:
	case AnyFX::SamplerCubeArray:
		return SamplerVariableType;
	case AnyFX::Texture1D:
	case AnyFX::Texture1DArray:
	case AnyFX::Texture2D:
	case AnyFX::Texture2DArray:
	case AnyFX::Texture2DMS:
	case AnyFX::Texture2DMSArray:
	case AnyFX::Texture3D:
	case AnyFX::TextureCube:
	case AnyFX::TextureCubeArray:
		return TextureVariableType;
	case AnyFX::TextureHandle:
		return TextureVariableType;
	case AnyFX::ImageHandle:
		return ImageReadWriteVariableType;
		break;
	case AnyFX::SamplerHandle:
		return SamplerVariableType;
	default:
		return ConstantBufferVariableType;
	}
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderPool::GetConstantName(const CoreGraphics::ShaderId id, const IndexT i) const
{
	AnyFX::VariableBase* var = this->shaderAlloc.Get<0>(id.allocId)->GetVariable(i);
	return var->name.c_str();
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderPool::GetConstantBufferCount(const CoreGraphics::ShaderId id) const
{
	return this->shaderAlloc.Get<0>(id.allocId)->GetNumVarblocks();
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkShaderPool::GetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i) const
{
	AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.allocId)->GetVarblock(i);
	return var->byteSize;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom
VkShaderPool::GetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i) const
{
	AnyFX::VarblockBase* var = this->shaderAlloc.Get<0>(id.allocId)->GetVarblock(i);
	return var->name.c_str();
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
VkShaderPool::GetResourceSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const
{
	const VkShaderSetupInfo& info = this->shaderAlloc.Get<1>(id.allocId);
	n_assert(info.resourceIndexMap.Contains(name));
	return info.resourceIndexMap[name];	
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>&
VkShaderPool::GetPrograms(const CoreGraphics::ShaderId id)
{
	return this->shaderAlloc.Get<2>(id.allocId).programMap;
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
VkShaderPool::GetProgramName(CoreGraphics::ShaderProgramId id)
{
	return this->shaderAlloc.Get<3>(id.shaderId).Get<0>(id.programId).name;
}

} // namespace Vulkan