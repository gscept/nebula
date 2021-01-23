//------------------------------------------------------------------------------
//  assertingmutex.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/debug.h"
#include "assertingmutex.h"
#include "interlocked.h"
namespace Threading
{

//------------------------------------------------------------------------------
/**
*/
AssertingMutex::AssertingMutex()
    : locked(false)
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
AssertingMutex::AssertingMutex(AssertingMutex&& rhs)
{
    Threading::Interlocked::Exchange(&this->locked, rhs.locked);
    Threading::Interlocked::Exchange(&rhs.locked, 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::operator=(AssertingMutex&& rhs)
{
    Threading::Interlocked::Exchange(&this->locked, rhs.locked);
    Threading::Interlocked::Exchange(&rhs.locked, 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::Lock()
{
    int old = Threading::Interlocked::Exchange(&this->locked, 1);
    n_assert(old == 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::Unlock()
{
    int old = Threading::Interlocked::Exchange(&this->locked, 0);
    n_assert(old != 0);
}

} // namespace Threading
