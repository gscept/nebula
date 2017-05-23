#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::SerialJobPort
    
    JobPort implementation in the serial job system. In the serial
    job system, PushJob() will immediately execute the job and return after
    the job has executed. All other methods are empty and return
    immediately.
    
    (C) 2009 Radon Labs GmbH
*/
#include "jobs/base/jobportbase.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class SerialJobPort : public Base::JobPortBase
{
    __DeclareClass(SerialJobPort);
public:
    /// constructor
    SerialJobPort();
    /// destructor
    virtual ~SerialJobPort();
    
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
};

} // namespace Jobs
//------------------------------------------------------------------------------
    