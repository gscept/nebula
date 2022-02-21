#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::LinuxCriticalSection

    On Linux, pthread mutexes are used for critical sections.
 
    @todo: Add debugging asserts? If yes wrap with new __NEBULA-define
 
    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include <mutex>
#include <atomic>

//------------------------------------------------------------------------------
namespace Linux
{
class LinuxCriticalSection
{
public:
    /// constructor
    LinuxCriticalSection();
    /// destructor
    ~LinuxCriticalSection();
    /// move assignment
    void operator=(LinuxCriticalSection&& rhs);
    /// enter the critical section
    void Enter() const;
    /// leave the critical section
    void Leave() const;
private:
    //mutable pthread_mutex_t mutex;
    mutable std::recursive_mutex mutex;
};

//------------------------------------------------------------------------------
/**
*/
inline
LinuxCriticalSection::LinuxCriticalSection()
{
    /*
    pthread_mutexattr_t mutexAttrs;
    pthread_mutexattr_init(&mutexAttrs);
    pthread_mutexattr_settype(&mutexAttrs, PTHREAD_MUTEX_RECURSIVE);    // allow nesting
    int res = pthread_mutex_init(&this->mutex, &mutexAttrs);
    n_assert(0 == res);
    pthread_mutexattr_destroy(&mutexAttrs);
    */
}

//------------------------------------------------------------------------------
/**
*/
inline 
LinuxCriticalSection::~LinuxCriticalSection()
{
    //int res = pthread_mutex_destroy(&this->mutex);
    //n_assert(0 == res);
}

//------------------------------------------------------------------------------
/**
*/
inline void
LinuxCriticalSection::operator=(LinuxCriticalSection&& rhs)
{
    this->mutex = std::move(rhs.mutex);
    rhs.mutex = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline void
LinuxCriticalSection::Enter() const
{
    //N_SCOPETRACETHIS();
    std::atomic_thread_fence(std::memory_order_acquire);
    mutex.lock();
    //pthread_mutex_lock(&this->mutex);
}
    
//------------------------------------------------------------------------------
/**
*/
inline void
LinuxCriticalSection::Leave() const
{
    //N_SCOPETRACETHIS();
    std::atomic_thread_fence(std::memory_order_release);
    mutex.unlock();
  
    //pthread_mutex_unlock(&this->mutex);
}

} // namespace Linux
