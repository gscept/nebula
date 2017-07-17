#pragma once
//------------------------------------------------------------------------------
/**
	The ResourceId type is just a StringAtom, but is primarily meant for resources.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "ids/id.h"
namespace Resources
{
typedef Util::StringAtom ResourceName;
typedef Ids::Id64 ResourceId;
} // namespace Resource