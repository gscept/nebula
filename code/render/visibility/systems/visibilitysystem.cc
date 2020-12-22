//------------------------------------------------------------------------------
//  system.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "visibilitysystem.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::PrepareObservers(const Math::mat4* transforms, Math::ClipStatus::Type* const* vis, const SizeT count)
{
    this->obs.transforms = transforms;
    this->obs.vis = vis;
    this->obs.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::PrepareEntities(const Math::mat4* transforms, const Graphics::GraphicsEntityId* entities, const bool* activeFlags, const SizeT count)
{
    this->ent.transforms = transforms;
    this->ent.entities = entities;
    this->ent.activeFlags = activeFlags;
    this->ent.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::Run()
{
    // do nothing
}

} // namespace Visibility
