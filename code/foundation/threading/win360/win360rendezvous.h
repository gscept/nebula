#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360Rendezvous
    
    A thread barrier for 2 threads using semaphores.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360Rendezvous
{
public:
    /// constructor
    Win360Rendezvous();
    /// destructor
    ~Win360Rendezvous();

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
Win360Rendezvous::Win360Rendezvous()
{
    this->semMaster = CreateSemaphore(NULL, 0, 1, NULL);
    this->semSlave = CreateSemaphore(NULL, 0, 1, NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline
Win360Rendezvous::~Win360Rendezvous()
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
Win360Rendezvous::MasterArrive()
{
    ReleaseSemaphore(this->semMaster, 1, NULL);
    WaitForSingleObject(this->semSlave, INFINITE);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Rendezvous::SlaveArrive()
{
    ReleaseSemaphore(this->semSlave, 1, NULL);
    WaitForSingleObject(this->semMaster, INFINITE);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Rendezvous::MasterReleaseSlave()
{
    // just call ReleaseSemaphore(), the max count can't exceed 1, so we're ok
    ReleaseSemaphore(this->semMaster, 1, NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Win360Rendezvous::SlaveReleaseMaster()
{
    // just call ReleaseSemaphore(), the max count can't exceed 1, so we're ok
    ReleaseSemaphore(this->semSlave, 1, NULL);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Win360Rendezvous::MasterHoldSlave()
{
	WaitForSingleObject(this->semSlave, INFINITE);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
Win360Rendezvous::SlaveHoldMaster()
{
	WaitForSingleObject(this->semMaster, INFINITE);
}
} // namespace Win360
//------------------------------------------------------------------------------
