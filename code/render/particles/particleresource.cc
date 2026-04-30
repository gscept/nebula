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
const ParticleEmitters&
ParticleResourceGetEmitters(const ParticleResourceId id)
{
    return particleResourceAllocator.Get<ParticleResource_Resource>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
ParticleEmitters&
ParticleResourceGetMutableEmitters(const ParticleResourceId id)
{
    return particleResourceAllocator.Get<ParticleResource_Resource>(id.id);
}

//------------------------------------------------------------------------------
/**
*/

} // namespace Particles
