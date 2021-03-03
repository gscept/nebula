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
#include "thread.h"

namespace Threading
{

enum RWAccessFlagBits
{
    NoAccess = 0x0,
    ReadAccess = 0x1,
    WriteAccess = 0x2,
    ReadWriteAccess = ReadAccess | WriteAccess
};
typedef int RWAccessFlags;

class ReadWriteLock
{
public:

    /// constructor
    ReadWriteLock();
    /// destructor
    ~ReadWriteLock();

    /// acquire lock
    void Acquire(const RWAccessFlags accessFlags);
    /// release lock
    void Release(const RWAccessFlags accessFlags);
private:
    volatile int numReaders;
    volatile ThreadId writerThread;

    Threading::CriticalSection writerSection;
    Threading::CriticalSection readerSection;
};  

} // namespace Threading
