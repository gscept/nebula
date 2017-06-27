//------------------------------------------------------------------------------
// graphicsentity.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsentity.h"
#include "graphicsserver.h"

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

	// register entity, this will give us the transform
	GraphicsServer::Instance()->RegisterEntity(this);
}

//------------------------------------------------------------------------------
/**
*/
GraphicsEntity::~GraphicsEntity()
{
	// unregister entity, after this point the transform is given to some other entity
	GraphicsServer::Instance()->UnregisterEntity(this);
}

} // namespace Graphics