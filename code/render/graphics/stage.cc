//------------------------------------------------------------------------------
// stage.cc
// (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "stage.h"

namespace Graphics
{

__ImplementClass(Graphics::Stage, 'STAG', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Stage::Stage()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Stage::~Stage()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
Stage::AttachEntity(const GraphicsEntityId entity)
{
	this->entities.Append(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
Stage::DetachEntity(const GraphicsEntityId entity)
{
	this->entities.EraseIndex(this->entities.FindIndex(entity));
}

} // namespace Graphics