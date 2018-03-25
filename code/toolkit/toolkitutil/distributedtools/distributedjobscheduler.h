#pragma once
//------------------------------------------------------------------------------
/**
	@class DistributedTools::DistributedJobScheduler
    
    The distributed job scheduler can launch the applications, which are defined
    in a DistributedTools::DistributedJob class. The scheduler can accept a list
    of jobs at one time. After ataching jobs to the scheduler the RunJobs() 
    method have to be called.

    If the scheduler uses remote services, jobs can sent over network to
    slave clients. If a service reports an error there is no way to make sure
    that sent jobs have finsihed properly. The scheduler assumes that all
    running jobs of that service were not finished and Re-assigns those jobs
    to the ready-jobs queue.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "distributedtools/remotejobservice.h"

namespace IO
{
    class TextWriter;
}
//------------------------------------------------------------------------------
namespace DistributedTools
{
class DistributedJob;

class DistributedJobScheduler{
public:
        
    /// constructor
    DistributedJobScheduler();

    /// opens the scheduler
    virtual bool Open();
    /// close the scheduler
    virtual void Close();
    /// is scheduler open?
    bool IsOpen();
    /// should the scheduler use remote services?
    void UseRemoteServices(bool flag);
    /// Set maximal count of parallel running local jobs
    void SetMaxParallelLocalJobs(int count);
    /// Attach a remote job service that can handle jobs
    virtual void AttachRemoteJobService(const Ptr<RemoteJobService> & service);

    /// attach a job that is ready to run
    virtual void AttachJob(const Ptr<DistributedJob> & job);
    /// run attached jobs
    virtual void RunJobs();
    /// update scheduler, checks for finished jobs and writes output
    virtual void Update();
    /// is any job still running?
    bool HasActiveJobs();

    /// Append an initialize-job
    void AppendInitializeJob(const Ptr<DistributedJob> & job);
    /// Get initialize-job list
    Util::Array<Ptr<DistributedJob>> GetInitializeJobList();
    /// Append an finlaize-job
    void AppendFinalizeJob(const Ptr<DistributedJob> & job);
    /// Get Finalize-job list
    Util::Array<Ptr<DistributedJob>> GetFinalizeJobList();
    /// should the sheduler print the local job output (default true)
    void PrintLocalJobOutput(bool enable);

private:
    /// checks ready jobs list and adds new job to local or remote lists
    void StartReadyJobs();
    /// adds a ready job to local running jobs if possible
    void StartReadyJobsLocal();
    /// adds a ready job to remote running jobs if possible
    void StartReadyJobsRemote();
    /// process local job
    void ProcessLocalJob(const Ptr<DistributedJob> & job);
    /// has running remote services
    bool HasRunningRemoteServices();
    /// send service to black list and handle its running jobs
    void DisableRunningService(const Ptr<RemoteJobService> & service);
    /// sends a message to a banned service and removes service from blacklist in some cases
    void HandleBannedService(const Ptr<RemoteJobService> & service);
    /// updates local job lists assuming specifies service is not running jobs for this scheduler
    void ResetJobsFromService(const Ptr<RemoteJobService> & service);
    
    Util::Array<Ptr<DistributedJob>> readyJobs;
    Util::Array<Ptr<DistributedJob>> localRunningJobs;
    Util::Array<Ptr<RemoteJobService>> remoteServices;
    Util::Array<Ptr<RemoteJobService>> bannedRemoteServices;
    Util::Array<Ptr<DistributedJob>> remoteRunningJobs;
    Util::Array<Ptr<DistributedJob>> initializeJobs;
    Util::Array<Ptr<DistributedJob>> finalizeJobs;
    Util::Array<Ptr<DistributedJob>> remoteInitializeJobs;
    Util::Array<Ptr<DistributedJob>> remoteFinalizeJobs;

    bool isRunningJobs;
    bool isOpen;
    int maxParallelLocalJob;
    bool useRemoteServices;
    bool printLocalJobOutput;
};
//------------------------------------------------------------------------------
/**
    Is scheduler open?	
*/
inline
bool
DistributedJobScheduler::IsOpen()
{
    return this->isOpen;
}
//------------------------------------------------------------------------------
/**
    Use remote services?	
*/
inline
void
DistributedJobScheduler::UseRemoteServices(bool flag)
{
    this->useRemoteServices = flag;
}

//------------------------------------------------------------------------------
/**
    maximal parallel local jobs	
*/
inline
void
DistributedJobScheduler::SetMaxParallelLocalJobs(int count)
{
    this->maxParallelLocalJob = count;
}

//------------------------------------------------------------------------------
/**
    Append a job as an initialize-job	
*/
inline
void
DistributedJobScheduler::AppendInitializeJob(const Ptr<DistributedJob> & job)
{
    this->initializeJobs.Append(job);
}
//------------------------------------------------------------------------------
/**
    Get list of all initialize-jobs	
*/
inline
Util::Array<Ptr<DistributedJob>>
DistributedJobScheduler::GetInitializeJobList()
{
    return this->initializeJobs;
}
//------------------------------------------------------------------------------
/**
    Append a job as an finalize-job	
*/
inline
void
DistributedJobScheduler::AppendFinalizeJob(const Ptr<DistributedJob> & job)
{
    this->finalizeJobs.Append(job);
}

//------------------------------------------------------------------------------
/**
    Get list of all finalize-jobs	
*/
inline
Util::Array<Ptr<DistributedJob>>
DistributedJobScheduler::GetFinalizeJobList()
{
    return this->finalizeJobs;
}

//------------------------------------------------------------------------------
/**
	If enabled the scheduler prints the output of the local jobs.
    (Remote job outputs are printed anyway)
*/
inline
void
DistributedJobScheduler::PrintLocalJobOutput(bool enable)
{
    this->printLocalJobOutput = enable;
}

} // namespace DistributedTools