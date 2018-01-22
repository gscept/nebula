#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystem
    
    A ParticleSystem object holds the shared attributes for all 
    its ParticleSystemInstances.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "particles/emitterattrs.h"
#include "particles/envelopesamplebuffer.h"
#include "particles/emittermesh.h"
#include "coregraphics/mesh.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace Particles
{
class ParticleSystem : public Core::RefCounted
{
    __DeclareClass(ParticleSystem);
public:
    /// constructor
    ParticleSystem();
    /// destructor
    virtual ~ParticleSystem();

    /// the default particle system update step time
    static const Timing::Time DefaultStepTime;
    /// the particle system update step time
    static Timing::Time StepTime;
    /// modulate stepTime by a factor, needed for time effects
    static void ModulateStepTime(float val);

    /// setup the particle system object
    void Setup(const CoreGraphics::MeshId emitterMesh, IndexT primGroupIndex, const EmitterAttrs& emitterAttrs);
    /// discard the particle system object
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;

    /// get pointer to emitter mesh
    const EmitterMesh& GetEmitterMesh() const;
    /// get access to particle emitter attributes
    const EmitterAttrs& GetEmitterAttrs() const;
    /// get access to pre-sampled envelope curve buffer
    const EnvelopeSampleBuffer& GetEnvelopeSampleBuffer() const;
    /// get the maximum number of particles alive
    SizeT GetMaxNumParticles() const;

private:    
    EmitterAttrs emitterAttrs;
    EnvelopeSampleBuffer envelopeSampleBuffer;
    EmitterMesh emitterMesh;
    bool isValid;
	bool isRestarted;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ParticleSystem::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline const EmitterMesh&
ParticleSystem::GetEmitterMesh() const
{
    return this->emitterMesh;
}

//------------------------------------------------------------------------------
/**
*/
inline const EmitterAttrs&
ParticleSystem::GetEmitterAttrs() const
{
    return this->emitterAttrs;
}

//------------------------------------------------------------------------------
/**
*/
inline const EnvelopeSampleBuffer&
ParticleSystem::GetEnvelopeSampleBuffer() const
{
    return this->envelopeSampleBuffer;
}

} // namespace Particles
//------------------------------------------------------------------------------
    