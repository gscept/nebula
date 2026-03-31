//------------------------------------------------------------------------------
//  @file particleresource.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "particleresource.h"
namespace Particles
{

ParticleResourceAllocator particleResourceAllocator;
//------------------------------------------------------------------------------
/**
*/
SizeT
ParticleResourceGetNumEmitters(const ParticleResourceId id)
{
    const ParticleEmitter& emitter = particleResourceAllocator.Get<ParticleResource_Resource>(id.id);
    return emitter.emitters.Size();
}

//------------------------------------------------------------------------------
/**
*/
const Particles::EmitterAttrs&
ParticleResourceGetEmitterAttrs(const ParticleResourceId id, const IndexT index)
{
    const ParticleEmitter& emitter = particleResourceAllocator.Get<ParticleResource_Resource>(id.id);
    return emitter.emitters[index];
}

} // namespace Particles
