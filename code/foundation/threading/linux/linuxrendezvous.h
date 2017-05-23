#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::LinuxRendezvous
    
    A thread barrier for 2 threads using semaphores.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include <semaphore.h>

//------------------------------------------------------------------------------
namespace Linux
{
class LinuxRendezvous
{
public:
    /// constructor
    LinuxRendezvous();
    /// destructor
    ~LinuxRendezvous();

    /// call for "master" thread
    void MasterArrive();
    /// call for "slave" thread
    void SlaveArrive();

private:
    sem_t semMaster;
    sem_t semSlave;
};

//------------------------------------------------------------------------------
/**
*/
inline
LinuxRendezvous::LinuxRendezvous()
{
    int res;
    res = sem_init(&this->semMaster, 0, 0);
    n_assert(0 == res);
    res = sem_init(&this->semSlave, 0, 0);
    n_assert(0 == res);
}

//------------------------------------------------------------------------------
/**
*/
inline
LinuxRendezvous::~LinuxRendezvous()
{
    sem_destroy(&this->semMaster);
    sem_destroy(&this->semSlave);
}

//------------------------------------------------------------------------------
/**
*/
inline void
LinuxRendezvous::MasterArrive()
{
    n_assert(sem_post(&this->semMaster) == 0);
    n_assert(sem_wait(&this->semSlave) == 0);
}

//------------------------------------------------------------------------------
/**
*/
inline void
LinuxRendezvous::SlaveArrive()
{
    n_assert(sem_post(&this->semSlave) == 0);
    n_assert(sem_wait(&this->semMaster) == 0);
}

} // namespace Linux
//------------------------------------------------------------------------------
