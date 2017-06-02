//------------------------------------------------------------------------------
// visibilityserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibilityserver.h"

namespace Visibility
{

__ImplementClass(Visibility::VisibilityServer, 'VISE', Core::RefCounted);
__ImplementSingleton(Visibility::VisibilityServer);
//------------------------------------------------------------------------------
/**
*/
VisibilityServer::VisibilityServer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisibilityServer::~VisibilityServer()
{
	__DestructSingleton;
}

} // namespace Visibility