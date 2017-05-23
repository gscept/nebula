#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::JobPortBase
  
    A JobPort accepts Jobs for execution and is used to wait for the 
    completion of jobs or to synchronize the execution of jobs which
    depend on each other.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "jobs/job.h"

//------------------------------------------------------------------------------
namespace Base
{
class JobPortBase : public Core::RefCounted
{
    __DeclareClass(JobPortBase);
public:
    /// constructor
    JobPortBase();
    /// destructor
    virtual ~JobPortBase();
    
    /// setup the job port
    void Setup();
    /// discard the job port
    void Discard();
    /// return true if the job object is valid
    bool IsValid() const;

    /// push a job for execution
    void PushJob(const Ptr<Jobs::Job>& job);
    /// push a job chain, each job in the chain depends on previous job
    void PushJobChain(const Util::Array<Ptr<Jobs::Job> >& jobs);
    /// push a flush command (makes sure that jobs don't re-use uniform data from previous jobs)
    void PushFlush();
    /// push a sync command (waits for completion of all previous jobs on this port)
    void PushSync();
    
    /// wait for completion
    void WaitDone();
    /// check for completion, return immediately
    bool CheckDone();

protected:
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
JobPortBase::IsValid() const
{
    return this->isValid;
}

} // namespace Base
//------------------------------------------------------------------------------
