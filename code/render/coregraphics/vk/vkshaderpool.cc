//------------------------------------------------------------------------------
// vkstreamshaderloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderpool.h"
#include "vkshader.h"
#include "coregraphics/constantbuffer.h"
#include "effectfactory.h"
#include "coregraphics/config.h"
#include "coregraphics/shaderstate.h"

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
VkShaderPool::Load(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());
	n_assert(stream->CanBeMapped());
	n_assert(!this->GetState(id) == Resources::Resource::Pending);

	// map stream to memory
	stream->SetAccessMode(Stream::ReadAccess);
	if (stream->Open())
	{
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

		SetupInfo& setupInfo = this->shaderAlloc.Get<1>(id);
		RuntimeInfo& runtimeInfo = this->shaderAlloc.Get<2>(id);

		this->shaderAlloc.Get<0>(id) = effect;
		setupInfo.id = ShaderIdentifier::FromName(this->GetName(id));
		setupInfo.name = this->GetName(id);
		VkShader::Setup(
			effect,
			setupInfo.constantRangeLayout,
			setupInfo.setBindings,
			setupInfo.immutableSamplers,
			setupInfo.descriptorSetLayouts,
			setupInfo.pipelineLayout,
			setupInfo.sets,
			setupInfo.uniformBufferMap,
			setupInfo.uniformBufferGroupMap
			);

		// setup shader variations
		const eastl::vector<AnyFX::ProgramBase*> programs = effect->GetPrograms();
		for (uint i = 0; i < programs.size(); i++)
		{
			// get program object from shader subsystem
			VkShaderProgram::ProgramAllocator& programAllocator = this->shaderAlloc.Get<3>(id);
			AnyFX::VkProgram* program = static_cast<AnyFX::VkProgram*>(programs[i]);
			Ids::Id32 programId = programAllocator.AllocResource();
			VkShaderProgram::Setup(programId, program, setupInfo.pipelineLayout, this->shaderAlloc.Get<3>(id));
			Ids::Id64 shaderProgramId = Ids::Id::MakeId64(id, programId);
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
	return ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::Unload(const Ids::Id24 res)
{
	//VkShader::Unload();
	//VkShader* shd = (VkShader*)res;
	//shd->Unload();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::BindShader(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask)
{
	Ids::Id24 shaderId = Ids::Id::GetLow(Ids::Id::GetBig(shaderId));
	RuntimeInfo& runtime = this->shaderAlloc.Get<2>(shaderId);
	Ids::Id64& programId = runtime.activeShaderProgram;
	VkShaderProgram::ProgramAllocator& programs = this->shaderAlloc.Get<3>(shaderId);

	// change variation if it's actually changed
	if (this->activeShaderProgram != programId && runtime.activeMask != mask)
	{
		programId = runtime.programMap[mask];
		runtime.activeMask = mask;
	}
	this->activeShaderProgram = programId;
	VkShaderProgram::Apply(programs.Get<2>(Ids::Id::GetLow(programId)));
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::BindShader(const CoreGraphics::ShaderProgramId shaderProgramId)
{
	Ids::Id24 shaderId = Ids::Id::GetHigh(shaderProgramId);
	Ids::Id32 programId = Ids::Id::GetLow(shaderProgramId);
	this->activeShaderProgram = shaderProgramId;
	VkShaderProgram::ProgramAllocator& programs = this->shaderAlloc.Get<3>(shaderId);
	VkShaderProgram::Apply(programs.Get<2>(programId));
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderProgramId
VkShaderPool::GetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask)
{
	Ids::Id24 shaderId = Ids::Id::GetLow(Ids::Id::GetBig(shaderId));
	RuntimeInfo& runtime = this->shaderAlloc.Get<2>(shaderId);
	return runtime.programMap[mask];
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderStateId
VkShaderPool::CreateState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups, bool createUniqueSet)
{
	// allocate a slice and create an id which is the shader id, and the state id
	const Ids::Id24 shaderId = Ids::Id::GetLow(Ids::Id::GetBig(shader));
	VkShaderState::ShaderStateAllocator& stateAllocator = this->shaderAlloc.Get<4>(shaderId);
	Ids::Id32 stateId = stateAllocator.AllocResource();
	SetupInfo& info = this->shaderAlloc.Get<1>(shaderId);
	VkShaderState::Setup(
		stateId,
		this->shaderAlloc.Get<0>(shaderId),
		groups,
		stateAllocator,
		info.sets,
		info.descriptorSetLayouts,
		createUniqueSet
		);

	// the resource id is the shader id and the state id combined
	CoreGraphics::ShaderStateId ret = Ids::Id::MakeId64(shaderId, stateId);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderStateId
VkShaderPool::CreateSharedState(const CoreGraphics::ShaderId shader, const Util::Array<IndexT>& groups)
{
	// allocate a slice and create an id which is the shader id, and the state id
	const Ids::Id24 shaderId = Ids::Id::GetLow(Ids::Id::GetBig(shader));
	Ids::Id32 stateId = -1;
	if (this->sharedStateMap.Contains(shaderId)) stateId = this->sharedStateMap[shaderId];
	else
	{
		VkShaderState::ShaderStateAllocator& stateAllocator = this->shaderAlloc.Get<4>(shaderId);
		stateId = stateAllocator.AllocResource();
		SetupInfo& info = this->shaderAlloc.Get<1>(shaderId);
		VkShaderState::Setup(
			stateId,
			this->shaderAlloc.Get<0>(shaderId),
			groups,
			stateAllocator,
			info.sets,
			info.descriptorSetLayouts,
			false
		);
		this->sharedStateMap.Add(shaderId, stateId);
	}
	n_assert(stateId != -1);

	// the resource id is the shader id and the state id combined
	CoreGraphics::ShaderStateId ret = Ids::Id::MakeId64(shaderId, stateId);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderPool::DestroyState(const CoreGraphics::ShaderStateId state)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	this->shaderAlloc.Get<4>(shaderId).DeallocResource(stateId);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderVariableId
VkShaderPool::GetShaderVariable(const CoreGraphics::ShaderStateId state, const Util::StringAtom& name)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	return this->shaderAlloc.Get<4>(shaderId).Get<1>(stateId).variableMap[name];
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::ShaderVariableId
VkShaderPool::GetShaderVariable(const CoreGraphics::ShaderStateId state, const IndexT index)
{
	Ids::Id32 shaderId = Ids::Id::GetHigh(state);
	Ids::Id32 stateId = Ids::Id::GetLow(state);
	return this->shaderAlloc.Get<4>(shaderId).Get<1>(stateId).variableMap.ValueAtIndex(index);
}

//------------------------------------------------------------------------------
/**
	Use direct resource ids, not the State, Shader or Variable type ids
*/
AnyFX::VkProgram*
VkShaderPool::GetProgram(const Ids::Id24 shaderId, const Ids::Id24 programId)
{
	return this->shaderAlloc.Get<3>(shaderId).Get<1>(programId);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderPool::GetVariableCount(const CoreGraphics::ShaderId id)
{
	const Ids::Id24 shaderId = Ids::Id::GetLow(Ids::Id::GetBig(id));
	return this->shaderAlloc.Get<0>(shaderId)->GetNumVariables();
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderPool::GetVariableBlockCount(const CoreGraphics::ShaderId id)
{
	const Ids::Id24 shaderId = Ids::Id::GetLow(Ids::Id::GetBig(id));
	return this->shaderAlloc.Get<0>(shaderId)->GetNumVarblocks();
}

} // namespace Vulkan