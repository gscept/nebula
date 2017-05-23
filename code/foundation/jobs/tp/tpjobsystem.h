#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPJobSystem
    
    Implementation of JobSystem for jobs running in a CPU thread pool.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "jobs/base/jobsystembase.h"
#include "jobs/tp/tpjobthreadpool.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPJobSystem : public Base::JobSystemBase
{
    __DeclareClass(TPJobSystem);
public:
    /// constructor
    TPJobSystem();
    /// destructor
    virtual ~TPJobSystem();

    /// setup the job system
    void Setup();
    /// shutdown the job system
    void Discard();

private:
    friend class TPJobPort;

    /// get pointer to thread pool
    TPJobThreadPool* GetThreadPool();

    TPJobThreadPool threadPool;
};

//------------------------------------------------------------------------------
/**
*/
inline TPJobThreadPool*
TPJobSystem::GetThreadPool()
{
    return &(this->threadPool);
}

} // namespace Jobs
//------------------------------------------------------------------------------
    