//------------------------------------------------------------------------------
// modelserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelserver.h"
#include "resources/resourcemanager.h"
#include "model.h"
#include "nodes/modelnode.h"

namespace Models
{

__ImplementClass(Models::ModelServer, 'MOSE', Core::RefCounted);
__ImplementSingleton(ModelServer);
//------------------------------------------------------------------------------
/**
*/
ModelServer::ModelServer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ModelServer::~ModelServer()
{
	__DestructSingleton;
}

} // namespace Models