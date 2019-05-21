#pragma once
//------------------------------------------------------------------------------
/**
	Particle context controls playing and enabling/disabling of particle emitters
	inside a model.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "models/model.h"
#include "models/nodes/modelnode.h"
#include "jobs/jobs.h"
namespace Particles
{

class ParticleContext : public Graphics::GraphicsContext
{
	_DeclareContext();
public:

	enum PlayMode
	{
		RestartIfPlaying,
		IgnoreIfPlaying
	};

	/// constructor
	ParticleContext();
	/// destrucotr
	virtual ~ParticleContext();

	/// create particle context
	static void Create();
	
	/// setup particle context on model
	static void Setup(const Graphics::GraphicsEntityId id);

	/// show particle based on index fetched from GetParticleId
	static void ShowParticle(const Graphics::GraphicsEntityId id, const IndexT particleId);
	/// hide particle
	static void HideParticle(const Graphics::GraphicsEntityId id, const IndexT particleId);
	/// get particle node id by name
	static const IndexT GetParticleId(const Graphics::GraphicsEntityId id, const Util::StringAtom& name);

	/// start playing particle
	static void Play(const Graphics::GraphicsEntityId id, const PlayMode mode);
	/// stop playing a particle
	static void Stop(const Graphics::GraphicsEntityId id);	

	/// start particle updating when frame starts
	static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks);
	/// stop particle updating when frame ends
	static void OnWaitForWork(const IndexT frameIndex, const Timing::Time frameTime);

#ifndef PUBLIC_DEBUG    
	/// debug rendering
	static void OnRenderDebug(uint32_t flags);
#endif

private:

	struct ParticleRuntime
	{
		Timing::Time stepTime;
		Timing::Time prevEmissionTime;
		Timing::Time emissionStartTimeOffset;
		IndexT emissionCounter : 30;
		bool firstFrame : 1;
		bool playing : 1;
	};

	enum
	{
		ParticleNodeMap,
		ModelId,
		Runtime
	};
	typedef Ids::IdAllocator<
		Util::Dictionary<Util::StringAtom, Models::ModelNode::Instance*>,
		Graphics::ContextEntityId,
		ParticleRuntime
	> ParticleContextAllocator;
	static ParticleContextAllocator particleContextAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);

	static Jobs::JobPortId jobPort;
	static Jobs::JobSyncId jobSync;
};

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId 
ParticleContext::Alloc()
{
	return particleContextAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Dealloc(Graphics::ContextEntityId id)
{
	particleContextAllocator.DeallocObject(id.id);
}

} // namespace Particles
