#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ParticleSystemInstanceBase
    
    The per-instance object of a ParticleSystem. This is where actual
    particles are created and updated.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "particles/particlesystem.h"
#include "particles/particlesystemstate.h"
#include "particles/particle.h"
#include "math/matrix44.h"
#include "util/ringbuffer.h"
#include "timing/time.h"
#include "jobs/job.h"
#include "particles/particlerenderinfo.h"

namespace Jobs { class JobPort; }

//------------------------------------------------------------------------------
namespace Particles
{

typedef Util::RingBuffer<Particles::Particle> ParticleBuffer;

class ParticleSystemInstanceBase : public Core::RefCounted
{
    __DeclareAbstractClass(ParticleSystemInstanceBase);
public:
    /// constructor
    ParticleSystemInstanceBase();
    /// destructor
    virtual ~ParticleSystemInstanceBase();

    /// setup the particle system instance
    virtual void Setup(const Ptr<CoreGraphics::Mesh>& emitterMesh, IndexT primGroupIndex, const EmitterAttrs& emitterAttrs);
    /// discard the particle system instance
    virtual void Discard();
    /// return true if the object has been setup
    bool IsValid() const;

    /// get pointer to particle system object
    const Ptr<Particles::ParticleSystem>& GetParticleSystem() const;
    /// set current world-space transform
    void SetTransform(const Math::matrix44& m);
    /// get current world-space transform
    const Math::matrix44& GetTransform() const;
    /// set current world-space velocity
    void SetVelocity(const Math::vector& v);
    /// get current world-space velocity
    const Math::vector& GetVelocity() const;
    /// set local wind vector
    void SetWindVector(const Math::vector& v);
    /// get local wind vector
    const Math::vector& GetWindVector() const;
    
    /// start emitting particles
    void Start();
    /// stop emitting particles
    void Stop();
	/// restart emitting particles
	void Restart();

    /// currently emitting particles?
    bool IsPlaying() const;
    /// stopped, but some particles still alive?
    bool IsStopping() const;
    /// stopped, and no particles alive?
    bool IsStopped() const;
    /// get current state mask
    Particles::ParticleSystemState::Mask GetState() const;

    /// update the particle system instance, call once per frame
    virtual void Update(Timing::Time time);
    /// debug-visualize the particle system
    void RenderDebug();
    /// get the current bounding box (valid after Update())
    const Math::bbox& GetBoundingBox() const;
    /// generate vertex streams to render
    virtual void UpdateVertexStreams();
    /// render it
    virtual void Render() = 0;
    /// before rendering happens
    virtual void OnRenderBefore();
    /// get the render info
    const Particles::ParticleRenderInfo& GetParticleRenderInfo() const;

private:
    /// emit new particles for the current frame
    void EmitParticles(float stepTime);
    /// emit a new single particle at the current position and time
    void EmitParticle(IndexT emissionSampleIndex, float initialAge);
    /// update the per-frame state of the particle system (Playing, Stopping, etc...)
    void UpdateState(float time);

protected:
    /// prepare pre-frame update of the job's uniform data for next job
    virtual void PrepareJobUniformData(float stepTime, bool generateVertexList, Jobs::JobUniformDesc &uniformDesc);
    /// prepare pre-frame update of the uniform for next job
    virtual void PrepareJobOutputData(bool generateVertexList, int inputBufferSize, Jobs::JobDataDesc &outputDesc);
    /// perform a single time-step-update on particles as jobs
    virtual void StartJobStepParticles(float stepTime, bool generateVertexList);
    /// collect job data 
    virtual void FinalizeJobs();

private:
    Math::matrix44 transform;
    Math::vector velocity;
    Math::bbox boundingBox;
    Timing::Time curStepTime;
    Timing::Time lastEmissionTime;
    Timing::Time timeSinceEmissionStart;
    IndexT emissionCounter;
    bool firstEmissionFrame;
    static JOB_ID jobIdCounter;
    IndexT particleId;

protected:
	ParticleSystemState::Mask stateMask;
	ParticleSystemState::Mask stateChangeMask;
	Ptr<CoreGraphics::Mesh> emitterMesh;
	IndexT primGroupIndex;
	EmitterAttrs emitterAttrs;

protected:
    JOB_ID jobId;
    uint numLivingParticles;
protected:
    Ptr<ParticleSystem> particleSystem;
    JobUniformData jobUniformData; 
    ParticleBuffer particles;
    ParticleRenderInfo renderInfo;
    struct JobData
    {
        bool running;
        Ptr<Jobs::JobPort> jobPort;
        Ptr<Jobs::Job> job;               
        // slices for current job
        int sliceCount;        
        // number of JobSliceOutputData's which can be stored 
        // in sliceOutput
        int sliceOutputCapacity;
        // output per slice
        // arrays capacity is stored in sliceOutputCapacity
        JobSliceOutputData *sliceOutput;
    } jobData;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ParticleSystem>&
ParticleSystemInstanceBase::GetParticleSystem() const
{
    return this->particleSystem;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ParticleSystemInstanceBase::IsValid() const
{
    return this->particleSystem.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ParticleSystemInstanceBase::IsPlaying() const
{
    return 0 != (ParticleSystemState::Playing & this->stateMask);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ParticleSystemInstanceBase::IsStopping() const
{
    return 0 != (ParticleSystemState::Stopping & this->stateMask);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ParticleSystemInstanceBase::IsStopped() const
{
    return 0 != (ParticleSystemState::Stopped & this->stateMask);
}

//------------------------------------------------------------------------------
/**
*/
inline ParticleSystemState::Mask
ParticleSystemInstanceBase::GetState() const
{
    return this->stateMask;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleSystemInstanceBase::SetTransform(const Math::matrix44& m)
{
    this->transform = m;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
ParticleSystemInstanceBase::GetTransform() const
{
    return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleSystemInstanceBase::SetVelocity(const Math::vector& v)
{
    this->velocity = v;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vector&
ParticleSystemInstanceBase::GetVelocity() const
{
    return this->velocity;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleSystemInstanceBase::SetWindVector(const Math::vector& v)
{
    this->jobUniformData.windVector = v;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vector&
ParticleSystemInstanceBase::GetWindVector() const
{
    return this->jobUniformData.windVector;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox&
ParticleSystemInstanceBase::GetBoundingBox() const
{
    return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
*/
inline const ParticleRenderInfo& 
ParticleSystemInstanceBase::GetParticleRenderInfo() const
{
    return this->renderInfo;
}

} // namespace Particles
//------------------------------------------------------------------------------
    
