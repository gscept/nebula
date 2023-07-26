#pragma once
//------------------------------------------------------------------------------
/**
    Posix implemention of a read-many write-few lock

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <pthread.h>
namespace Posix
{

class PosixReadWriteLock
{
public:
    /// Constructor
    PosixReadWriteLock();
    /// Destructor
    ~PosixReadWriteLock();

    /// Lock for read
    void LockRead();
    /// Lock for write
    void LockWrite();

    /// Release read
    void UnlockRead();
    /// Release write
    void UnlockWrite();

private:
    int32_t readCounter = 0;
    int32_t writeCounter = 0;
    pthread_rwlock_t lock;
    pthread_t lockingThread = 0;
};

} // namespace Posix
