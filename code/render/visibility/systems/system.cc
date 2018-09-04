//------------------------------------------------------------------------------
//  system.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void
System::PrepareObservers(const Math::matrix44* transforms, const bool* vis, const SizeT count)
{
	this->obs.transforms = transforms;
	this->obs.vis = vis;
	this->obs.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
System::PrepareEntities(const Math::matrix44* transforms, Graphics::GraphicsEntityId* entities, const SizeT count)
{
	this->ent.transforms = transforms;
	this->ent.entities = entities;
	this->ent.count = count;
}

//------------------------------------------------------------------------------
/**
*/
void
System::Run()
{
	// do nothing
}

} // namespace Visibility
