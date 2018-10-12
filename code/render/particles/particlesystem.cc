//------------------------------------------------------------------------------
//  particlesystem.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/particlesystem.h"
#include "particles/particle.h"

namespace Particles
{
__ImplementClass(Particles::ParticleSystem, 'PSYS', Core::RefCounted);

using namespace CoreGraphics;

const Timing::Time ParticleSystem::DefaultStepTime = 1.0f / 60.0f;
Timing::Time ParticleSystem::StepTime = 1.0f / 60.0f;

//------------------------------------------------------------------------------
/**
*/
ParticleSystem::ParticleSystem() :
    isValid(false),
	isRestarted(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ParticleSystem::~ParticleSystem()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystem::Setup(const CoreGraphics::MeshId mesh, IndexT primGrpIndex, const EmitterAttrs& attrs)
{
    n_assert(!this->IsValid());

    this->emitterAttrs = attrs;
    this->envelopeSampleBuffer.Setup(attrs, ParticleSystemNumEnvelopeSamples);
    this->emitterMesh.Setup(mesh, primGrpIndex);
    this->isValid = true;	
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystem::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
    this->emitterMesh.Discard();
    this->envelopeSampleBuffer.Discard();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ParticleSystem::GetMaxNumParticles() const
{
    float maxFreq = this->emitterAttrs.GetEnvelope(EmitterAttrs::EmissionFrequency).GetMaxValue();
    float maxLifeTime = this->emitterAttrs.GetEnvelope(EmitterAttrs::LifeTime).GetMaxValue();
    return 1 + SizeT(maxFreq * maxLifeTime);
}

//------------------------------------------------------------------------------
/**
*/
void 
ParticleSystem::ModulateStepTime(float val)
{
    ParticleSystem::StepTime = val * ParticleSystem::DefaultStepTime;        
}
} // namespace Particles