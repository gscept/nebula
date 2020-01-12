//------------------------------------------------------------------------------
//  particlecontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particlecontext.h"
#include "graphics/graphicsserver.h"
#include "models/modelcontext.h"
#include "models/nodes/particlesystemnode.h"
#include "particles/emitterattrs.h"
#include "particles/emittermesh.h"
#include "particles/envelopesamplebuffer.h"

using namespace Graphics;
using namespace Models;
namespace Particles
{

ParticleContext::ParticleContextAllocator ParticleContext::particleContextAllocator;
_ImplementContext(ParticleContext, ParticleContext::particleContextAllocator);

extern void ParticleStepJob(const Jobs::JobFuncContext& ctx);


const Timing::Time DefaultStepTime = 1.0f / 60.0f;
Timing::Time StepTime = 1.0f / 60.0f;

const SizeT ParticleContextNumEnvelopeSamples = 192;

Jobs::JobPortId ParticleContext::jobPort;
Jobs::JobSyncId ParticleContext::jobSync;
Util::Queue<Jobs::JobId> ParticleContext::runningJobs;
//------------------------------------------------------------------------------
/**
*/
ParticleContext::ParticleContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ParticleContext::~ParticleContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Create()
{
	__bundle.OnPrepareView = ParticleContext::OnPrepareView;
	__bundle.OnWaitForWork = ParticleContext::OnWaitForWork;
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
	Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);

	// setup nodes
	const Util::Array<Models::ModelNode::Instance*>& nodes = Models::ModelContext::GetModelNodeInstances(mdlId);
	IndexT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		Models::ModelNode* node = nodes[i]->node;
		if (node->type == ParticleSystemNodeType)
		{
			ParticleSystemRuntime runtime;
			runtime.node = reinterpret_cast<Models::ParticleSystemNode::Instance*>(node);
			Models::ParticleSystemNode* pNode = reinterpret_cast<Models::ParticleSystemNode*>(runtime.node->node);
			const Particles::EmitterAttrs& attrs = pNode->GetEmitterAttrs();
			runtime.uniformData.sampleBuffer = pNode->GetSampleBuffer().GetSampleBuffer();
			runtime.uniformData.gravity = Math::float4(0.0f, attrs.GetFloat(EmitterAttrs::Gravity), 0.0f, 0.0f);
			runtime.uniformData.stretchToStart = attrs.GetBool(EmitterAttrs::StretchToStart);
			runtime.uniformData.stretchTime = attrs.GetBool(EmitterAttrs::StretchToStart);
			runtime.uniformData.windVector = attrs.GetFloat4(EmitterAttrs::WindDirection);
			systems.Append(runtime);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::ShowParticle(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);

	// use node map to set active flag
	Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);
	for (IndexT i = 0; i < systems.Size(); i++)
	{
		systems[i].node->active = true;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::HideParticle(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);

	// use node map to set active flag
	Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);
	for (IndexT i = 0; i < systems.Size(); i++)
	{
		systems[i].node->active = false;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Play(const Graphics::GraphicsEntityId id, const PlayMode mode)
{
	const ContextEntityId cid = GetContextId(id);

	// use node map to set active flag
	Util::Array<ParticleSystemRuntime>& systems = particleContextAllocator.Get<ParticleSystems>(cid.id);
	ParticleRuntime& runtime = particleContextAllocator.Get<Runtime>(cid.id);

	if ((runtime.playing && mode == RestartIfPlaying) || !runtime.playing)
	{
		runtime.stepTime = 0;
		runtime.prevEmissionTime = 0.0f;
		runtime.emissionStartTimeOffset = 0.0f;
		runtime.firstFrame = true;
		runtime.playing = true;
	}
	
	for (IndexT i = 0; i < systems.Size(); i++)
	{
		auto node = systems[i].node;
		if ((runtime.playing && mode == RestartIfPlaying) || !runtime.playing)
		{
			systems[i].particles.Reset();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::Stop(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);

	ParticleRuntime& runtime = particleContextAllocator.Get<Runtime>(cid.id);
	runtime.stopping = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::OnPrepareView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
	const Util::Array<ParticleRuntime>& runtimes = particleContextAllocator.GetArray<Runtime>();
	const Util::Array<Util::Array<ParticleSystemRuntime>>& allSystems = particleContextAllocator.GetArray<ParticleSystems>();

	IndexT i;
	for (i = 0; i < runtimes.Size(); i++)
	{
		ParticleRuntime& runtime = runtimes[i];
		const Util::Array<ParticleSystemRuntime>& systems = allSystems[i];

		Timing::Time timeDiff = ctx.time - runtime.stepTime;
		n_assert(timeDiff >= 0.0f);
		if (timeDiff > 0.5f)
		{
			runtime.stepTime = ctx.time - StepTime;
			timeDiff = StepTime;
		}

		if (runtime.playing)
		{
			IndexT j;
			for (j = 0; j < systems.Size(); j++)
			{
				ParticleSystemRuntime& system = systems[j];

#if NEBULA_USED_FIXED_PARTICLE_UPDATE_TIME
				IndexT curStep = 0;
				while (runtime.stepTime <= ctx.time)
				{
					ParticleContext::EmitParticles(runtime, system, ParticleContext::StepTime);
					bool isLastJob = (runtime.stepTime + ParticleContext::StepTime) > ctx.time;
					ParticleContext::RunParticleStep(runtime, system, ParticleContext::StepTime, isLastJob);
					runtime.stepTime += ParticleContext::StepTime;
					curStep++;
				}
#else
				if (!runtime.stopping)
				{
					ParticleContext::EmitParticles(runtime, system, float(timeDiff));
				}
				ParticleContext::RunParticleStep(runtime, system, float(timeDiff), true);
				runtime.stepTime += timeDiff;
#endif
			}
		}		
	}

	// issue sync
	Jobs::JobSyncSignal(jobSync, ParticleContext::jobPort);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::OnWaitForWork(const Graphics::FrameContext& ctx)
{
	if (ParticleContext::runningJobs.Size() > 0)
	{
		// wait for all jobs to finish
		Jobs::JobSyncHostWait(ParticleContext::jobSync);

		// destroy jobs
		while (!ParticleContext::runningJobs.IsEmpty())
			Jobs::DestroyJob(ParticleContext::runningJobs.Dequeue());
	}
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

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::EmitParticles(ParticleRuntime& rt, ParticleSystemRuntime& srt, float stepTime)
{
	// get the (wrapped around if looping) time since emission has started
	Models::ParticleSystemNode* node = reinterpret_cast<Models::ParticleSystemNode*>(srt.node->node);
	const Particles::EmitterAttrs& attrs = node->GetEmitterAttrs();
	const Particles::EmitterMesh& mesh = node->GetEmitterMesh();
	const Particles::EnvelopeSampleBuffer& buffer = node->GetSampleBuffer();
	rt.emissionStartTimeOffset += stepTime;
	Timing::Time emDuration = attrs.GetFloat(EmitterAttrs::EmissionDuration);
	Timing::Time startDelay = attrs.GetFloat(EmitterAttrs::StartDelay);
	Timing::Time loopTime = emDuration + startDelay;
	bool looping = attrs.GetBool(EmitterAttrs::Looping);
	if (looping && (rt.emissionStartTimeOffset > loopTime))
	{
		// a wrap-around
		rt.emissionStartTimeOffset = Math::n_fmod((float)rt.emissionStartTimeOffset, (float)loopTime);
	}

	// if we are before the start delay, we definitely don't need to emit anything
	if (rt.emissionStartTimeOffset < startDelay)
	{
		// we're before the start delay
		return;
	}
	else if ((rt.emissionStartTimeOffset > loopTime) && !looping)
	{
		// we're past the emission time
		if (0 == (rt.stopping | rt.stopping))
		{
			rt.stopped = 1;
		}
		return;
	}

	// compute the relative emission time (0.0 .. 1.0)
	Timing::Time relEmissionTime = (rt.emissionStartTimeOffset - startDelay) / emDuration;
	IndexT emSampleIndex = IndexT(relEmissionTime * (Particles::ParticleContextNumEnvelopeSamples - 1));

	// lookup current emission frequency
	float emFrequency = buffer.LookupSamples(emSampleIndex)[EmitterAttrs::EmissionFrequency];
	if (emFrequency > N_TINY)
	{
		Timing::Time emTimeStep = 1.0 / emFrequency;

		// if we haven't emitted particles in this run yet, we need to compute
		// the precalc-time and initialize the lastEmissionTime
		if (rt.firstFrame)
		{
			rt.firstFrame = false;

			// handle pre-calc if necessary, this will instantly produce
			// a number of particles to let the particle system
			// appear as if has been emitting for quite a while already
			float preCalcTime = attrs.GetFloat(EmitterAttrs::PrecalcTime);
			if (preCalcTime > N_TINY)
			{
				// during pre-calculation we need to update the particle system,
				// but we do this with a lower step-rate for better performance
				// (but less precision)
				rt.prevEmissionTime = 0.0f;
				Timing::Time updateTime = 0.0f;
				Timing::Time updateStep = 0.05f;
				if (updateStep < emTimeStep)
				{
					updateStep = emTimeStep;
				}
				while (rt.prevEmissionTime < preCalcTime)
				{
					ParticleContext::EmitParticle(rt, srt, attrs, mesh, buffer, emSampleIndex, 0.0f);
					rt.prevEmissionTime += emTimeStep;
					updateTime += emTimeStep;
					if (updateTime >= updateStep)
					{
						ParticleContext::RunParticleStep(rt, srt, (float)updateStep, false);
						updateTime = 0.0f;
					}
				}
			}

			// setup the lastEmissionTime for particle emission so that at least
			// one frame's worth of particles is emitted, this is necessary
			// for very short-emitting particles
			rt.prevEmissionTime = rt.stepTime;
			if (stepTime > emTimeStep)
			{
				rt.prevEmissionTime -= (stepTime + N_TINY);
			}
			else
			{
				rt.prevEmissionTime -= (emTimeStep + N_TINY);
			}
		}

		// handle the "normal" particle emission while the particle system is emitting
		while ((rt.prevEmissionTime + emTimeStep) <= rt.stepTime)
		{
			rt.prevEmissionTime += emTimeStep;
			ParticleContext::EmitParticle(rt, srt, attrs, mesh, buffer, emSampleIndex, (float)(rt.stepTime - rt.prevEmissionTime));
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::EmitParticle(ParticleRuntime& rt, ParticleSystemRuntime& srt, const Particles::EmitterAttrs& attrs, const Particles::EmitterMesh& mesh, const Particles::EnvelopeSampleBuffer& buffer, IndexT sampleIndex, float initialAge)
{
	n_assert(initialAge >= 0.0f);

	using namespace Math;

	Particle particle;
	float* particleEnvSamples = buffer.LookupSamples(0);
	float* emissionEnvSamples = buffer.LookupSamples(sampleIndex);

	// lookup pseudo-random emitter vertex from the emitter mesh
	const EmitterMesh::EmitterPoint& emPoint = mesh.GetEmitterPoint(srt.emissionCounter++);

	// setup particle position and start position
	particle.position = matrix44::transform(emPoint.position, srt.transform);
	particle.startPosition = particle.position;
	particle.stretchPosition = particle.position;

	// compute emission direction
	float minSpread = emissionEnvSamples[EmitterAttrs::SpreadMin];
	float maxSpread = emissionEnvSamples[EmitterAttrs::SpreadMax];
	float theta = n_deg2rad(n_lerp(minSpread, maxSpread, n_rand()));
	float rho = N_PI_DOUBLE * n_rand();
	matrix44 rot = matrix44::multiply(matrix44::rotationaxis(emPoint.tangent, theta), matrix44::rotationaxis(emPoint.normal, rho));
	float4 emNormal = matrix44::transform(emPoint.normal, rot);

	float4 dummy, dummy2;
	quaternion qrot;
	srt.transform.decompose(dummy, qrot, dummy2);
	emNormal = matrix44::transform(emNormal, matrix44::rotationquaternion(qrot));
	// compute start velocity
	float velocityVariation = 1.0f - (n_rand() * attrs.GetFloat(EmitterAttrs::VelocityRandomize));
	float startVelocity = emissionEnvSamples[EmitterAttrs::StartVelocity] * velocityVariation;

	// setup particle velocity vector
	particle.velocity = emNormal * startVelocity;

	// setup uvMinMax to a random texture tile
	// FIXME: what's up with the horizontal flip?
	float texTile = n_clamp(attrs.GetFloat(EmitterAttrs::TextureTile), 1.0f, 16.0f);
	float step = 1.0f / texTile;
	float tileIndex = floorf(n_rand() * texTile);
	float vMin = step * tileIndex;
	float vMax = vMin + step;
	particle.uvMinMax.set(1.0f, 1.0f - vMin, 0.0f, 1.0f - vMax);

	// setup initial particle color and clamp to 0,0,0
	particle.color.load(&(particleEnvSamples[EmitterAttrs::Red]));

	// setup rotation and rotationVariation
	float startRotMin = attrs.GetFloat(EmitterAttrs::StartRotationMin);
	float startRotMax = attrs.GetFloat(EmitterAttrs::StartRotationMax);
	particle.rotation = n_clamp(n_rand(), startRotMin, startRotMax);
	float rotVar = 1.0f - (n_rand() * attrs.GetFloat(EmitterAttrs::RotationRandomize));
	if (attrs.GetBool(EmitterAttrs::RandomizeRotation) && (n_rand() < 0.5f))
	{
		rotVar = -rotVar;
	}
	particle.rotationVariation = rotVar;

	// setup particle size and size variation
	particle.size = particleEnvSamples[EmitterAttrs::Size];
	particle.sizeVariation = 1.0f - (n_rand() * attrs.GetFloat(EmitterAttrs::SizeRandomize));

	// setup particle age and oneDivLifetime, clamp lifetime to 1.0f if there's a risk we will divide by 0
	float lifeTime = emissionEnvSamples[EmitterAttrs::LifeTime];
	if (lifeTime >= 1.0f)
	{
		particle.oneDivLifeTime = 1.0f / lifeTime;
	}
	else
	{
		particle.oneDivLifeTime = 0.0f;
	}
	particle.relAge = initialAge * particle.oneDivLifeTime;
	particle.age = initialAge;

	// group particles dependent on its lifetime
	if (particle.relAge < 0.25f)
	{
		particle.particleId = 4;
	}
	else if (particle.relAge < 0.5f)
	{
		particle.particleId = 3;
	}
	else if (particle.relAge < 0.75f)
	{
		particle.particleId = 2;
	}
	else
	{
		particle.particleId = 1;
	}
	//this->particleId = !this->particleId;
	// set particle id, just switch between 0 and 1 
	//particle.particleId = (float)this->particleId;    
	//if (++this->particleId > 3) this->particleId = 0;         

	// add the new particle to the particle ring buffer
	srt.particles.Add(particle);
}


//------------------------------------------------------------------------------
/**
*/
void 
ParticleContext::RunParticleStep(ParticleRuntime& rt, ParticleSystemRuntime& srt, float stepTime, bool generateVtxList)
{
	// if no particles, no need to run the step update
	if (srt.particles.Size() == 0)
		return;

	Jobs::JobContext ctx;

	n_assert(srt.particles.GetBuffer());
	const SizeT inputBufferSize = srt.particles.Size() * ParticleJobInputElementSize;
	const SizeT inputSliceSize = ParticleJobInputSliceSize;

	ctx.input.data[0] = srt.particles.GetBuffer();
	ctx.input.dataSize[0] = inputBufferSize;
	ctx.input.sliceSize[0] = inputSliceSize;
	ctx.input.numBuffers = 1;

	const SizeT outputSliceCount = (inputBufferSize + inputSliceSize - 1) / inputSliceSize;
	n_assert(outputSliceCount > 0);
	if (outputSliceCount > srt.outputCapacity)
	{
		if (srt.outputData)
			n_delete_array(srt.outputData);

		srt.outputData = n_new_array(ParticleJobOutput, outputSliceCount);
		srt.outputCapacity = outputSliceCount;
	}

	
	ctx.output.data[0] = srt.particles.GetBuffer();
	ctx.output.dataSize[0] = inputBufferSize;
	ctx.output.sliceSize[0] = inputSliceSize;

	ctx.output.data[1] = srt.outputData;
	ctx.output.dataSize[1] = outputSliceCount * sizeof(ParticleJobOutput);
	ctx.output.sliceSize[1] = sizeof(ParticleJobOutput);

	// issue job
	Jobs::JobId job = Jobs::CreateJob({ Particles::ParticleStepJob });

	// pass in uniforms from system which is not step-dependent
	ctx.uniform.data[0] = &srt.uniformData;
	ctx.uniform.dataSize[0] = sizeof(srt.uniformData);

	// create a copy of the uniforms, because the step size may change between every step,
	// which causes bugs if we do precalculation while we are changing the value
	srt.perJobUniformData.stepTime = stepTime;
	void* uniformCopy = Jobs::JobAllocateScratchMemory(job, Memory::HeapType::ScratchHeap, sizeof(ParticleJobUniformPerJobData));
	memcpy(uniformCopy, &srt.perJobUniformData, sizeof(ParticleJobUniformPerJobData));
	ctx.uniform.data[1] = uniformCopy;
	ctx.uniform.dataSize[1] = sizeof(ParticleJobUniformPerJobData);
	ctx.uniform.numBuffers = 2;

	// schedule job
	Jobs::JobSchedule(job, ParticleContext::jobPort, ctx);

	// add to list of running jobs
	ParticleContext::runningJobs.Enqueue(job);
}

} // namespace Particles
