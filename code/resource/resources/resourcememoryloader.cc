//------------------------------------------------------------------------------
// resourcememoryloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcememoryloader.h"

namespace Resources
{

__ImplementClass(Resources::ResourceMemoryLoader, 'RMLO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourceMemoryLoader::ResourceMemoryLoader()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceMemoryLoader::~ResourceMemoryLoader()
{
	// empty
}

} // namespace Resources