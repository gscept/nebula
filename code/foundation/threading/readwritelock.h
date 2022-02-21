#pragma once
//------------------------------------------------------------------------------
/**
    A read write lock is a type of thread synchronization primitive which supports for a 
    multiple consumers, single producer pattern. It means resources within the scope of this lock
    can be read by many threads, but can only be modified, written, by one.

    Acquire locks in the following way:
        For reads:
            If an acquire with write is currently holding the lock
            If we manage to run acquire just after we decrement the number of readers in release

        For writes:
            If an acquire with write is currently holding the lock
            If the lock is acquired by any reader

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/config.h"
#if (__WIN32__)
#include "threading/win32/win32readwritelock.h"
namespace Threading
{
class ReadWriteLock : public Win32::Win32ReadWriteLock
{
public:
    ReadWriteLock() : Win32::Win32ReadWriteLock()
    {};
};
}
#elif ( __linux__ || __OSX__ || __APPLE__ )
#include "threading/posix/posixreadwritelock.h"
namespace Threading
{
class ReadWriteLock : public Posix::PosixReadWriteLock
{
public:
    ReadWriteLock() : Posix::PosixReadWriteLock()
    {};
};
}
#else
#error "Threading::ReadWriteLock not implemented on this platform!"
#endif
