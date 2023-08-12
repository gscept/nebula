//------------------------------------------------------------------------------
//  @file win32readwritelock.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "win32readwritelock.h"

thread_local uint readCounter = 0;
thread_local uint writeCounter = 0;
namespace Win32
{

//------------------------------------------------------------------------------
/**
*/
Win32ReadWriteLock::Win32ReadWriteLock()
{
    InitializeSRWLock(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
Win32ReadWriteLock::~Win32ReadWriteLock()
{
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ReadWriteLock::LockRead()
{
    if (readCounter++ == 0)
        AcquireSRWLockShared(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ReadWriteLock::LockWrite()
{
    if (writeCounter++ == 0)
        AcquireSRWLockExclusive(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ReadWriteLock::UnlockRead()
{
    if (--readCounter == 0)
        ReleaseSRWLockShared(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ReadWriteLock::UnlockWrite()
{
    if (--writeCounter == 0)
        ReleaseSRWLockExclusive(&this->lock);
}

} // namespace Win32
