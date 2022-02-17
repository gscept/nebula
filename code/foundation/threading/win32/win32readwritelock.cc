//------------------------------------------------------------------------------
//  @file win32readwritelock.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "win32readwritelock.h"
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
    AcquireSRWLockShared(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ReadWriteLock::LockWrite()
{
    AcquireSRWLockExclusive(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ReadWriteLock::UnlockRead()
{
    ReleaseSRWLockShared(&this->lock);
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ReadWriteLock::UnlockWrite()
{
    ReleaseSRWLockExclusive(&this->lock);
}

} // namespace Win32
