//------------------------------------------------------------------------------
//  system.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibilitysystem.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::PrepareObservers(const Math::matrix44* transforms, bool* const* vis, const SizeT count)
{
	this->obs.transforms = transforms;
	this->obs.vis = vis;
	this->obs.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySystem::PrepareEntities(const Math::matrix44* transforms, Graphics::GraphicsEntityId* entities, const SizeT count)
{
	this->ent.transforms = transforms;
	this->ent.entities = entities;
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
