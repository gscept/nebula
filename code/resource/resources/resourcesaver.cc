//------------------------------------------------------------------------------
// resourcesaver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcesaver.h"

namespace Resources
{

__ImplementAbstractClass(Resources::ResourceSaver, 'RESA', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourceSaver::ResourceSaver()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceSaver::~ResourceSaver()
{
	// empty
}

} // namespace Resources