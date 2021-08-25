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
VisibilitySystem::PrepareEntities(const Math::bbox* boxes, const Graphics::GraphicsEntityId* entities, const uint32_t* entityFlags, const SizeT count)
{
    this->ent.boxes = boxes;
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
