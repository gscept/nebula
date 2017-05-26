//------------------------------------------------------------------------------
// resourcecontainer.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcecontainer.h"

namespace Resource
{

__ImplementClass(Resource::ResourceContainer, 'RECO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourceContainer::ResourceContainer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceContainer::~ResourceContainer()
{
	// empty
}

} // namespace Resource