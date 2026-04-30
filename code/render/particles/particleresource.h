#pragma once
//------------------------------------------------------------------------------
/**
    A ParticleResource contains the data for a particle effect, such as attrs and meshes

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

struct ParticleEmitters
{
    Util::Array<Util::StringAtom> name;
    Util::Array<Resources::ResourceId> meshes;
    Util::Array<Resources::ResourceId> materials;
    Util::Array<Math::mat4> transform;
    Util::Array<EmitterAttrs> attrs;
};

/// Get attrs
const ParticleEmitters& ParticleResourceGetEmitters(const ParticleResourceId id);

#if WITH_NEBULA_EDITOR
ParticleEmitters& ParticleResourceGetMutableEmitters(const ParticleResourceId id);
#endif

enum
{
    ParticleResource_Resource
};

typedef Ids::IdAllocatorSafe<
    0xFFFF,
    ParticleEmitters
> ParticleResourceAllocator;
extern ParticleResourceAllocator particleResourceAllocator;

} // namespace Particles
