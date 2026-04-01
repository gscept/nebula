#pragma once
//------------------------------------------------------------------------------
/**
    A ParticleResource contains the data for a particle effect, such as emitters and meshes

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/config.h"
#include "resources/resourceid.h"
#include "ids/id.h"
#include "ids/idallocator.h"
#include "particles/emitterattrs.h"

namespace Particles
{

RESOURCE_ID_TYPE(ParticleResourceId);

struct ParticleEmitter
{
    Util::Array<Resources::ResourceId> meshes;
    Resources::ResourceId albedo, material, normals;
    Math::mat4 transform;
    Util::Array<EmitterAttrs> emitters;
};

/// Get number of emitters in a particle resource
SizeT ParticleResourceGetNumEmitters(const ParticleResourceId id);
/// Get an emitter from a particle resource
const Particles::EmitterAttrs& ParticleResourceGetEmitterAttrs(const ParticleResourceId id, const IndexT index);

enum
{
    ParticleResource_Resource
};

typedef Ids::IdAllocatorSafe<
    0xFFFF,
    ParticleEmitter
> ParticleResourceAllocator;
extern ParticleResourceAllocator particleResourceAllocator;

} // namespace Particles
