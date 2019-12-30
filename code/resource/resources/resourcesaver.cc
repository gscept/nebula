//------------------------------------------------------------------------------
// resourcesaver.cc
// (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
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