//------------------------------------------------------------------------------
// graphicsentity.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsentity.h"

namespace Graphics
{

__ImplementClass(Graphics::GraphicsEntity, 'GREN', Core::RefCounted);
int64_t GraphicsEntity::UniqueIdCounter = 0;

//------------------------------------------------------------------------------
/**
*/
GraphicsEntity::GraphicsEntity()
{
	this->id = UniqueIdCounter++;
}

//------------------------------------------------------------------------------
/**
*/
GraphicsEntity::~GraphicsEntity()
{
	// empty
}

} // namespace Graphics