#pragma once
//------------------------------------------------------------------------------
/**
    A mutex object which will assert instead of lock the thread

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <atomic>
namespace Threading
{

class AssertingMutex
{
public:
    /// constructor
    AssertingMutex();
    /// destructor
    ~AssertingMutex();
    /// move constructor
    AssertingMutex(AssertingMutex&& rhs);
    /// copy constructor
    /// AssertingMutex(const AssertingMutex& rhs);
    /// move operator
    void operator=(AssertingMutex&& rhs);

    /// lock mutex
    void Lock();
    /// unlock mutex
    void Unlock();
private:
    std::atomic_bool locked;
};

struct AssertingScope
{
    /// constructor
    AssertingScope(AssertingMutex* mutex)
        : mutex(mutex)
    {
        mutex->Lock();
    }

    /// destructor
    ~AssertingScope()
    {
        mutex->Unlock();
    }

    AssertingMutex* mutex;
};

} // namespace Threading
