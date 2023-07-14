//------------------------------------------------------------------------------
//  assertingmutex.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/debug.h"
#include "assertingmutex.h"

namespace Threading
{

//------------------------------------------------------------------------------
/**
*/
AssertingMutex::AssertingMutex()
    : locked(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssertingMutex::~AssertingMutex()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::Lock()
{
    n_assert(++this->locked == 1);
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::Unlock()
{
    n_assert(--this->locked == 0);
}

} // namespace Threading
