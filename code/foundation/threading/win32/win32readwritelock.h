#pragma once
//------------------------------------------------------------------------------
/**
    Win32 implemention of a read-many write-few lock

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Win32
{

class Win32ReadWriteLock
{
public:
    /// Constructor
    Win32ReadWriteLock();
    /// Destructor
    ~Win32ReadWriteLock();

    /// Lock for read
    void LockRead();
    /// Lock for write
    void LockWrite();

    /// Release read
    void UnlockRead();
    /// Release write
    void UnlockWrite();

private:
    SRWLOCK lock;
};

} // namespace Win32
