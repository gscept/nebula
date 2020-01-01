//------------------------------------------------------------------------------
//  particlecontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "particlecontext.h"
#include "graphics/graphicsserver.h"
#include "models/modelcontext.h"

using namespace Graphics;
using namespace Models;
namespace Particles
{

ParticleContext::ParticleContextAllocator ParticleContext::particleContextAllocator;
_ImplementContext(ParticleContext, ParticleContext::particleContextAllocator);

Jobs::JobPortId ParticleContext::jobPort;
Jobs::JobSyncId ParticleContext::jobSync;
//------------------------------------------------------------------------------
/**
*/
ParticleContext::ParticleContext()
{
}

//------------------------------------------------------------------------------
/**
*/
ParticleContext::~ParticleContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Create()
{
	__bundle.OnBeforeFrame = ParticleContext::OnBeforeFrame;
	__bundle.OnWaitForWork = ParticleContext::OnWaitForWork;
	__bundle.OnBeforeView = nullptr;
	__bundle.OnAfterView = nullptr;
	__bundle.OnAfterFrame = nullptr;
	__bundle.StageBits = &ParticleContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = ParticleContext::OnRenderDebug;
#endif

	ParticleContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	Jobs::CreateJobPortInfo info =
	{
		"ParticleJobPort",
		2,
		System::Cpu::Core1 | System::Cpu::Core2,
		UINT_MAX
	};
	ParticleContext::jobPort = Jobs::CreateJobPort(info);

	Jobs::CreateJobSyncInfo sinfo =
	{
		nullptr
	};
	ParticleContext::jobSync = Jobs::CreateJobSync(sinfo);

	_CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Setup(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	n_assert_fmt(cid != ContextEntityId::Invalid(), "Entity %d is not registered in ParticleContext", id.HashCode());

	// get model context
	const ContextEntityId mdlId = Models::ModelContext::GetContextId(id);
	n_assert_fmt(mdlId != ContextEntityId::Invalid(), "Entity %d needs to be setup as a model before character!", id.HashCode());
	particleContextAllocator.Get<ModelId>(cid.id) = mdlId;

	// get node map
	Util::Dictionary<Util::StringAtom, Models::ModelNode::Instance*>& nodeMap = particleContextAllocator.Get<ParticleNodeMap>(cid.id);

	// setup nodes
	const Util::Array<Models::ModelNode::Instance*>& nodes = Models::ModelContext::GetModelNodeInstances(mdlId);
	IndexT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		const Models::ModelNode* node = nodes[i]->node;
		if (node->type == ParticleSystemNodeType)
		{
			nodeMap.Add(node->name, nodes[i]);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::ShowParticle(const Graphics::GraphicsEntityId id, const IndexT particleId)
{
	const ContextEntityId cid = GetContextId(id);

	// use node map to set active flag
	Util::Dictionary<Util::StringAtom, Models::ModelNode::Instance*>& nodeMap = particleContextAllocator.Get<ParticleNodeMap>(cid.id);
	nodeMap.ValueAtIndex(particleId)->active = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::HideParticle(const Graphics::GraphicsEntityId id, const IndexT particleId)
{
	const ContextEntityId cid = GetContextId(id);

	// use node map to set active flag
	Util::Dictionary<Util::StringAtom, Models::ModelNode::Instance*>& nodeMap = particleContextAllocator.Get<ParticleNodeMap>(cid.id);
	nodeMap.ValueAtIndex(particleId)->active = false;
}


//------------------------------------------------------------------------------
/**
*/
const IndexT 
ParticleContext::GetParticleId(const Graphics::GraphicsEntityId id, const Util::StringAtom& name)
{
	const ContextEntityId cid = GetContextId(id);

	// get node map
	const Util::Dictionary<Util::StringAtom, Models::ModelNode::Instance*>& nodeMap = particleContextAllocator.Get<ParticleNodeMap>(cid.id);
	IndexT i = nodeMap.FindIndex(name);
	n_assert(i != InvalidIndex);
	return i;
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Play(const Graphics::GraphicsEntityId id, const PlayMode mode)
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Stop(const Graphics::GraphicsEntityId id)
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks)
{
	const Util::Array<ParticleRuntime>& runtimes = particleContextAllocator.GetArray<Runtime>();

	IndexT i;
	for (i = 0; i < runtimes.Size(); i++)
	{
		const ParticleRuntime& runtime = runtimes[i];
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::OnWaitForWork(const IndexT frameIndex, const Timing::Time frameTime)
{
}

#ifndef PUBLIC_DEBUG    
//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::OnRenderDebug(uint32_t flags)
{
}
#endif

} // namespace Particles
