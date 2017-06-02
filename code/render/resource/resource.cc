//------------------------------------------------------------------------------
// resource.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resource.h"

namespace Resources
{

__ImplementClass(Resources::Resource, 'RESO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Resource::Resource()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Resource::~Resource()
{
	// empty
}

} // namespace Resources