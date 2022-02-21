//------------------------------------------------------------------------------
//  @file posixreadwritelock.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/debug.h"
#include "posixreadwritelock.h"
namespace Posix
{

//------------------------------------------------------------------------------
/**
*/
PosixReadWriteLock::PosixReadWriteLock()
{
    int ret = pthread_rwlock_init(&this->lock, nullptr);
    n_assert(ret == 0);
}

//------------------------------------------------------------------------------
/**
*/
PosixReadWriteLock::~PosixReadWriteLock()
{
    int ret = pthread_rwlock_destroy(&this->lock);
    n_assert(ret == 0);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixReadWriteLock::LockRead()
{
    pthread_rwlock_rdlock(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixReadWriteLock::LockWrite()
{
    pthread_rwlock_wrlock(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixReadWriteLock::UnlockRead()
{
    pthread_rwlock_unlock(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixReadWriteLock::UnlockWrite()
{
    pthread_rwlock_unlock(&this->lock);
}

} // namespace Posix
