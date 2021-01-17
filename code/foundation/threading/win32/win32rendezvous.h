#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Rendezvous
    
    A thread barrier for 2 threads using semaphores.
    
    @copyright
    (C) 2010 Radon Labs GmbH
    @copyright
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Rendezvous
{
public:
    /// constructor
    Win32Rendezvous();
    /// destructor
    ~Win32Rendezvous();

    /// call for "master" thread
    void MasterArrive();
    /// call for "slave" thread
    void SlaveArrive();
    /// called by "master" thread to "unstuck" slave thread
    void MasterReleaseSlave();
    /// called by "slave" thread to "unstuck" master thread
    void SlaveReleaseMaster();
    /// called by "master" thread to "stick" slave thread
    void MasterHoldSlave();
    /// called by "slave" thread to "stick" master thread
    void SlaveHoldMaster();

private:
    HANDLE semMaster;
    HANDLE semSlave;
};

//------------------------------------------------------------------------------
/**
*/
inline
Win32Rendezvous::Win32Rendezvous()
{
    this->semMaster = CreateSemaphore(NULL, 0, 1, NULL);
    this->semSlave = CreateSemaphore(NULL, 0, 1, NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline
Win32Rendezvous::~Win32Rendezvous()
{
    CloseHandle(this->semMaster);
    CloseHandle(this->semSlave);
    this->semMaster = 0;
    this->semSlave = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win32Rendezvous::MasterArrive()
{
    ReleaseSemaphore(this->semMaster, 1, NULL);
    WaitForSingleObject(this->semSlave, INFINITE);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win32Rendezvous::SlaveArrive()
{
    ReleaseSemaphore(this->semSlave, 1, NULL);
    WaitForSingleObject(this->semMaster, INFINITE);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win32Rendezvous::MasterReleaseSlave()
{
    // just call ReleaseSemaphore(), the max count can't exceed 1, so we're ok
    ReleaseSemaphore(this->semMaster, 1, NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win32Rendezvous::SlaveReleaseMaster()
{
    // just call ReleaseSemaphore(), the max count can't exceed 1, so we're ok
    ReleaseSemaphore(this->semSlave, 1, NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Win32Rendezvous::MasterHoldSlave()
{
    WaitForSingleObject(this->semSlave, INFINITE);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Win32Rendezvous::SlaveHoldMaster()
{
    WaitForSingleObject(this->semMaster, INFINITE);
}
} // namespace Win32
//------------------------------------------------------------------------------
