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
    this->locked.store(rhs.locked.load());
    rhs.locked.exchange(false);
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::operator=(AssertingMutex&& rhs)
{
    this->locked.store(rhs.locked.load());
    rhs.locked.exchange(false);
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::Lock()
{
    bool res = this->locked.exchange(true);
    n_assert(!res);
}

//------------------------------------------------------------------------------
/**
*/
void 
AssertingMutex::Unlock()
{
    bool res = this->locked.exchange(false);
    n_assert(res);
}

} // namespace Threading
