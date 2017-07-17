//------------------------------------------------------------------------------
// modelserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelserver.h"

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

//------------------------------------------------------------------------------
/**
*/
Models::ModelServer::NodeInstance*
ModelServer::CreateNodeInstance()
{
	Ids::Id64 id;
	NodeInstance* inst = this->nodeInstancePool.Alloc(id);
	inst->id = id;
	inst->primitiveGroupId = 0;
	inst->surface = nullptr;
	return inst;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelServer::DiscardNodeInstance(NodeInstance* node)
{
	this->nodeInstancePool.Free(node->id);
}

} // namespace Models