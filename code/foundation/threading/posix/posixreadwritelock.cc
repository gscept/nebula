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
    int ret = pthread_rwlock_rdlock(&this->lock);
    n_assert(ret == 0);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixReadWriteLock::LockWrite()
{
    int ret = pthread_rwlock_trywrlock(&this->lock);
    if (ret == EBUSY)
    {
        ret = pthread_rwlock_wrlock(&this->lock);
    }
    writeCounter++;
    lockingThread = pthread_self();
    n_assert(ret == 0 || ret == EDEADLK);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixReadWriteLock::UnlockRead()
{
    int ret = pthread_rwlock_unlock(&this->lock);
    n_assert(ret == 0);
}

//------------------------------------------------------------------------------
/**
*/
void
PosixReadWriteLock::UnlockWrite()
{
    n_assert(lockingThread == pthread_self());
    if (--writeCounter == 0)
    {
        int ret = pthread_rwlock_unlock(&this->lock);
        n_assert(ret == 0);
    }
}

} // namespace Posix
