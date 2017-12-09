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
ID_32_24_8_TYPE(ResourceId);			// 32 first bits are the loaders container id, 24 next bits is the shared resource, and last 8 bits is the type of loader

} // namespace Resource