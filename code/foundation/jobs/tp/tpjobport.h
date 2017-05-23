#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPJobPort
  
    Thread-pool implementation of JobPort.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "jobs/base/jobportbase.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPJobPort : public Base::JobPortBase
{
    __DeclareClass(TPJobPort);
public:
    /// constructor
    TPJobPort();
    /// destructor
    virtual ~TPJobPort();
    
    /// discard the job port
    void Discard();
    
    /// push a job for execution
    void PushJob(const Ptr<Job>& job);
    /// push a job chain, each job in the chain depends on previous job
    void PushJobChain(const Util::Array<Ptr<Jobs::Job> >& jobs);
    /// push a flush command (no effect in thread-pool job system)
    void PushFlush();
    /// push a sync command
    void PushSync();

    /// wait for completion
    void WaitDone();
    /// check for completion, return immediately
    bool CheckDone();

private:
    Ptr<Job> lastPushedJob;     // pointer to last pushed job
};

} // namespace Jobs
//------------------------------------------------------------------------------

