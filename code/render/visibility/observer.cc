//------------------------------------------------------------------------------
// observer.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "observer.h"

namespace Visibility
{

__ImplementClass(Visibility::Observer, 'OBSE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Observer::Observer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
Observer::~Observer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
Observer::OnVisibilityDatabaseChanged()
{

}


} // namespace Visibility