#pragma once
//------------------------------------------------------------------------------
/**
	Particle context controls playing and enabling/disabling of particle emitters
	inside a model.

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "models/model.h"
#include "models/nodes/modelnode.h"
#include "models/nodes/particlesystemnode.h"
#include "jobs/jobs.h"
#include "util/ringbuffer.h"
#include "particle.h"
namespace Particles
{

class EmitterAttrs;
class EmitterMesh;
class EnvelopeSampleBuffer;
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
	static void ShowParticle(const Graphics::GraphicsEntityId id);
	/// hide particle
	static void HideParticle(const Graphics::GraphicsEntityId id);

	/// start playing particle
	static void Play(const Graphics::GraphicsEntityId id, const PlayMode mode);
	/// stop playing a particle
	static void Stop(const Graphics::GraphicsEntityId id);	

	/// start particle updating when frame starts
	static void UpdateParticles(const Graphics::FrameContext& ctx);
	/// stop particle updating when frame ends
	static void WaitForParticleUpdates(const Graphics::FrameContext& ctx);

	/// get the shared particle index buffer
	static CoreGraphics::IndexBufferId GetParticleIndexBuffer();
	/// get the shared vertex buffer
	static CoreGraphics::VertexBufferId GetParticleVertexBuffer();
	/// get the shared vertex layout
	static CoreGraphics::VertexLayoutId GetParticleVertexLayout();

	/// get particle bounding box
	static Math::bbox GetBoundingBox(const Graphics::GraphicsEntityId id);

#ifndef PUBLIC_DEBUG    
	/// debug rendering
	static void OnRenderDebug(uint32_t flags);
#endif

	static CoreGraphics::MeshId DefaultEmitterMesh;

private:

	struct ParticleRuntime
	{
		Timing::Time stepTime;
		Timing::Time prevEmissionTime;
		Timing::Time emissionStartTimeOffset;
		IndexT emissionCounter : 30;
		bool initial : 1;
		bool playing : 1;
		bool stopping : 1;
		bool stopped : 1;
		bool restarting : 1;
		bool firstFrame : 1;
	};

	struct ParticleJobOutput
	{
		Math::bbox bbox;
		unsigned int numLivingParticles;
	};

	struct ParticleSystemRuntime
	{
		Models::ParticleSystemNode::Instance* node;
		Util::RingBuffer<Particle> particles;
		Math::mat4 transform;
		Math::bbox boundingBox;
		SizeT emissionCounter;

		ParticleJobUniformPerJobData perJobUniformData;
		ParticleJobUniformData uniformData;
		SizeT outputCapacity;
		ParticleJobOutput* outputData;
	};

	enum
	{
		ParticleSystems,
		ModelId,
		Runtime,
		BoundingBox
	};
	typedef Ids::IdAllocator<
		Util::Array<ParticleSystemRuntime>,
		Graphics::ContextEntityId,
		ParticleRuntime,
		Math::bbox
	> ParticleContextAllocator;
	static ParticleContextAllocator particleContextAllocator;

	/// internal function for emitting new particles
	static void EmitParticles(ParticleRuntime& rt, ParticleSystemRuntime& srt, float stepTime);
	/// internal function for emitting single particle
	static void EmitParticle(ParticleRuntime& rt, ParticleSystemRuntime& srt, const Particles::EmitterAttrs& attrs, const Particles::EmitterMesh& mesh, const Particles::EnvelopeSampleBuffer& buffer, IndexT sampleIndex, float initialAge);
	/// internal function to emit a job for updating particles
	static void RunParticleStep(ParticleRuntime& rt, ParticleSystemRuntime& srt, float stepTime, bool generateVtxList);

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);

	static Jobs::JobPortId jobPort;
	static Jobs::JobSyncId jobSync;
	static Util::Queue<Jobs::JobId> runningJobs;
};

//------------------------------------------------------------------------------
/**
*/
inline 
Graphics::ContextEntityId 
ParticleContext::Alloc()
{
	return particleContextAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
inline
void 
ParticleContext::Dealloc(Graphics::ContextEntityId id)
{
	particleContextAllocator.Dealloc(id.id);
}

} // namespace Particles
