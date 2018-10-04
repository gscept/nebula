#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::OSXCriticalSection

    On OSX, pthread mutexes are used for critical sections.
 
    @todo: Add debugging asserts? If yes wrap with new __NEBULA-define
 
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace OSX
{
class OSXCriticalSection
{
public:
    /// constructor
    OSXCriticalSection();
    /// destructor
    ~OSXCriticalSection();
    /// enter the critical section
    void Enter() const;
    /// leave the critical section
    void Leave() const;
private:
    mutable pthread_mutex_t mutex;
};

//------------------------------------------------------------------------------
/**
*/
inline
OSXCriticalSection::OSXCriticalSection()
{
    pthread_mutexattr_t mutexAttrs;
    pthread_mutexattr_init(&mutexAttrs);
    pthread_mutexattr_settype(&mutexAttrs, PTHREAD_MUTEX_RECURSIVE);    // allow nesting
    int res = pthread_mutex_init(&this->mutex, &mutexAttrs);
    n_assert(0 == res);
    pthread_mutexattr_destroy(&mutexAttrs);
}

//------------------------------------------------------------------------------
/**
*/
inline 
OSXCriticalSection::~OSXCriticalSection()
{
    int res = pthread_mutex_destroy(&this->mutex);
    n_assert(0 == res);
}

//------------------------------------------------------------------------------
/**
*/
inline void
OSXCriticalSection::Enter() const
{
    pthread_mutex_lock(&this->mutex);
}
    
//------------------------------------------------------------------------------
/**
*/
inline void
OSXCriticalSection::Leave() const
{
    pthread_mutex_unlock(&this->mutex);
}

} // namespace OSX