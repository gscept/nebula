//------------------------------------------------------------------------------
//  distributedjobscheduler.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "distributedtools/distributedjobscheduler.h"
#include "distributedtools/distributedjobs/distributedjob.h"
#include "io/console.h"
#include "io/textwriter.h"

using namespace IO;
using namespace Util;
using namespace Net;
//------------------------------------------------------------------------------
namespace DistributedTools
{
//------------------------------------------------------------------------------
/**
    Constructor	
*/
DistributedJobScheduler::DistributedJobScheduler() :
    isRunningJobs(false),
    maxParallelLocalJob(2),
    isOpen(false),
    useRemoteServices(false),
    printLocalJobOutput(true)
{
}

//------------------------------------------------------------------------------
/**
    Open the scheduler	
*/
bool
DistributedJobScheduler::Open()
{
    n_assert(!this->isOpen);   
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the scheduler	
*/
void
DistributedJobScheduler::Close()
{
    n_assert(this->isOpen);

    this->bannedRemoteServices.Clear();
    this->finalizeJobs.Clear();
    this->initializeJobs.Clear();
    this->localRunningJobs.Clear();
    this->readyJobs.Clear();
    this->remoteFinalizeJobs.Clear();
    this->remoteInitializeJobs.Clear();
    this->remoteRunningJobs.Clear();
    this->remoteServices.Clear();
    this->isRunningJobs = false;

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
    Attach a job to the ready job list
*/
void
DistributedJobScheduler::AttachJob(const Ptr<DistributedJob> & job)
{
    n_assert(this->isOpen);
    n_assert(!this->isRunningJobs);
    this->readyJobs.Append(job);
}

//------------------------------------------------------------------------------
/**
    Attach a remote job service that is used by this scheduler to process
    jobs.
*/
void
DistributedJobScheduler::AttachRemoteJobService(const Ptr<RemoteJobService> & service)
{
    n_assert(this->isOpen);
    n_assert(!this->isRunningJobs);     
    this->remoteServices.Append(service);
}

//------------------------------------------------------------------------------
/**
    Run the attached jobs. Returns immediately.
*/
void
DistributedJobScheduler::RunJobs()
{
    n_assert(this->isOpen);
    n_assert(!this->isRunningJobs);
    
    // set up initialize and finalize job lists
    this->remoteInitializeJobs.Clear();
    this->remoteFinalizeJobs.Clear();
    IndexT job;
    for(job = 0; job < this->initializeJobs.Size(); job++)
    {
        this->remoteInitializeJobs.Append(this->initializeJobs[job]->Clone());
    }
    for(job = 0; job < this->finalizeJobs.Size(); job++)
    {
        this->remoteFinalizeJobs.Append(this->finalizeJobs[job]->Clone());
    }

    // start local jobs immediately and begin to send remote jobs
    // to connected slaves
    this->StartReadyJobs();
}

//------------------------------------------------------------------------------
/**
    Updates the scheduler.
    
    First all initialize-jobs will start sequentially on the local machine.

    Then, after all initialize-jobs have finished, it process all standard jobs
    by starting them local or sending them to assigned remote services.

    After all standard jobs have finished, all finalize-jobs will be processed
    sequentially on the local machine.

    If a job has finished (remote or local) new ready jobs will started.

    Upates the isRunningJobs flag.
*/
void
DistributedJobScheduler::Update()
{
    n_assert(this->isOpen);
    n_assert(this->isRunningJobs);

    // becomes true, if a local or remote job finished
    bool jobFinished = false;
    
    // run initialize jobs first if there are any
    if (this->initializeJobs.Size()>0)
    {
        // process the job
        this->ProcessLocalJob(this->initializeJobs[0]);
        // print output
        if (this->printLocalJobOutput && this->initializeJobs[0]->HasOutputContent())
        {
            Console::Instance()->Print(
                "\n\n[Local output]\n\n%s\n\n",
                this->initializeJobs[0]->DequeueOutputContent().AsCharPtr());
        }
        // check state
        if(DistributedJob::Finished == this->initializeJobs[0]->GetCurrentState())
        {
            n_printf("[Local initialize-job #%s] Finished\n",this->initializeJobs[0]->GetIdentifier().AsCharPtr());
            this->initializeJobs.EraseIndex(0);
        }
        else if(DistributedJob::JobError == this->initializeJobs[0]->GetCurrentState())
        {
            n_printf("[Local initialize-job #%s] FAILED\n",this->initializeJobs[0]->GetIdentifier().AsCharPtr());            
            this->initializeJobs.EraseIndex(0);
            Console::Instance()->Error("A local initialize-job failed.");
        }
        // if all initial jobs are finished, set the jobFinished Flag,
        // so a new StartReadyJobs is called...
        if(this->initializeJobs.Size()==0)
        {
            jobFinished = true;
        }
        else
        {
            // else return and process left initial jobs
            return;
        }
    }

    // read local jobs output and put it to the output stream
    // remove finsihed jobs from list
    IndexT i=0;
    while (i<this->localRunningJobs.Size())
    {
        // process job
        this->ProcessLocalJob(this->localRunningJobs[i]);
        // print putput
        if (this->printLocalJobOutput && this->localRunningJobs[i]->HasOutputContent())
        {
            Console::Instance()->Print(
                "\n\n[Local output]\n\n%s\n\n",
                this->localRunningJobs[i]->DequeueOutputContent().AsCharPtr());
        }
        // check state
        if (DistributedJob::Finished == this->localRunningJobs[i]->GetCurrentState())
        {
            n_printf("[Local job #%s] Finished\n",this->localRunningJobs[i]->GetIdentifier().AsCharPtr());
            this->localRunningJobs.EraseIndex(i);
            jobFinished = true;
        }
        else if (DistributedJob::JobError == this->localRunningJobs[i]->GetCurrentState())
        {
            n_printf("[Local job #%s] FAILED\n",this->localRunningJobs[i]->GetIdentifier().AsCharPtr());            
            this->localRunningJobs.EraseIndex(i);
            jobFinished = true;
            Console::Instance()->Print("A local job failed.");
        }
        else
        {
            i++;
        }
    }
    
    if (this->useRemoteServices)
    {
        // update remote job services
        IndexT s;
        for (s = 0; s < this->remoteServices.Size(); s++)
        {
            if (this->bannedRemoteServices.FindIndex(this->remoteServices[s]) == InvalidIndex)
            {
                if (RemoteJobService::ServiceError!=this->remoteServices[s]->GetServiceState())
                {
                    if (this->remoteServices[s]->IsOpen())
                    {
                        this->remoteServices[s]->Update();
                    }
                    
                    // handle finished jobs
                    Util::Array<Util::Guid> finishedJobs = this->remoteServices[s]->GetFinishedJobGuids();
                    IndexT guid;
                    for (guid = 0; guid < finishedJobs.Size(); guid++)
                    {
            	        IndexT i = 0;
            	        while (i < this->remoteRunningJobs.Size())
            	        {
            		        if (this->remoteRunningJobs[i]->GetGuid() == finishedJobs[guid])
                            {
                                n_printf("[Remote job #%s] Finished\n",this->remoteRunningJobs[i]->GetIdentifier().AsCharPtr());
                                this->remoteRunningJobs.EraseIndex(i);
                                jobFinished = true;
                            }
                            else
                            {
                                i++;
                            }
            	        }
                    }
                    
                    // handle soft failed jobs
                    Util::Array<Util::Guid> failedJobs = this->remoteServices[s]->GetFailedJobGuids();
                    for (guid = 0; guid < failedJobs.Size(); guid++)
                    {
                        IndexT i = 0;
                        while (i < this->remoteRunningJobs.Size())
                        {
                            if (this->remoteRunningJobs[i]->GetGuid() == failedJobs[guid])
                            {
                                // check if job is in initialize or finalize joblist
                                bool wasInitializeFinalizeJob = false;
                                IndexT j = 0;
                                while (i < this->remoteServices[s]->GetInitializeJobs().Size())
                                {
                                    if (this->remoteServices[s]->GetInitializeJobs()[j]->GetGuid() == this->remoteRunningJobs[i]->GetGuid())
                                    {
                                        wasInitializeFinalizeJob = true;
                                    }
                                    else
                                    {
                                        j++;
                                    }
                                }
                                j=0;
                                while (i < this->remoteServices[s]->GetFinalizeJobs().Size())
                                {
                                    if (this->remoteServices[s]->GetFinalizeJobs()[j]->GetGuid() == this->remoteRunningJobs[i]->GetGuid())
                                    {
                                        wasInitializeFinalizeJob = true;
                                    }
                                    else
                                    {
                                        j++;
                                    }
                                }
                                if (wasInitializeFinalizeJob)
                                {
                                    n_printf("[Remote Initialize/Finalize job #%s] Failed to start. reset service...\n",this->remoteRunningJobs[i]->GetIdentifier().AsCharPtr());
                                    Console::Instance()->Error("A remote initialize or finalize job was in an error state.");
                                }
                                else
                                {
                                    n_printf("[Remote job #%s] Failed to start. Retrying later...\n",this->remoteRunningJobs[i]->GetIdentifier().AsCharPtr());
                                    this->ResetJobsFromService(this->remoteServices[s]);
                                    this->remoteServices[s]->RemoveFromFailedJobList(failedJobs[guid]);
                                }
                            }
                            else
                            {
                                i++;
                            }
                        }
                    }
                }
                else
                {
                    Console::Instance()->Warning(("A service reported an error:\n"+this->remoteServices[s]->GetErrorMessage()).AsCharPtr());
                    // Handle services in error state
                    Console::Instance()->Print("\nBanned service on %s.\n",this->remoteServices[s]->GetIpAddress().GetHostName().AsCharPtr());
                    this->DisableRunningService(this->remoteServices[s]);
                    // tell the scheduler a job has finished, so new ready jobs will be started
                    jobFinished = true;
                }
            }
            else
            {
                this->HandleBannedService(this->remoteServices[s]);
            }
        }
    }
    // start new jobs if any was finished
    if(jobFinished)
    {
        this->StartReadyJobs();
    }
    
    // run finalize jobs if there is no local and remote job left
    if (
        this->finalizeJobs.Size()>0 &&
        this->localRunningJobs.Size() == 0 &&
        this->remoteRunningJobs.Size() == 0
        )
    {
        this->ProcessLocalJob(this->finalizeJobs[0]);
        // print output
        if (this->printLocalJobOutput && this->finalizeJobs[0]->HasOutputContent())
        {
            Console::Instance()->Print(
                "\n\n[Local output]\n\n%s\n\n",
                this->finalizeJobs[0]->DequeueOutputContent().AsCharPtr());
        }
        // check state
        if (DistributedJob::Finished == this->finalizeJobs[0]->GetCurrentState())
        {
            n_printf("[Local finalize-job #%s] Finished\n",this->finalizeJobs[0]->GetIdentifier().AsCharPtr());
            this->finalizeJobs.EraseIndex(0);
        }
        else if (DistributedJob::JobError == this->finalizeJobs[0]->GetCurrentState())
        {
            n_printf("[Local finalize-job #%s] FAILED\n",this->finalizeJobs[0]->GetIdentifier().AsCharPtr());            
            this->finalizeJobs.EraseIndex(0);
            Console::Instance()->Error("A local finalize-job failed.");
        }
        // return and process finalize jobs until all are finished
        return;
    }
    
    // check the isRunning state
    if(
        this->HasRunningRemoteServices() ||
        this->localRunningJobs.Size() > 0 ||
        this->remoteRunningJobs.Size() > 0 ||
        this->finalizeJobs.Size() > 0 ||
        this->initializeJobs.Size() > 0)
    {
        this->isRunningJobs = true;
    }
    else
    {
        this->isRunningJobs = false;
    } 
}

//------------------------------------------------------------------------------
/**
    Is any job still running?	
*/
bool
DistributedJobScheduler::HasActiveJobs()
{
    n_assert(this->isOpen);
    return this->isRunningJobs;
}

//------------------------------------------------------------------------------
/**
    Starts new jobs from ready jobs list.
    Clears the initialize/finalize job list
*/
void
DistributedJobScheduler::StartReadyJobs()
{
    n_assert(this->isOpen);
    DistributedJobScheduler::StartReadyJobsLocal();
    if (this->useRemoteServices)
    {
        DistributedJobScheduler::StartReadyJobsRemote();
    }

    // At this point there have to be running jobs if ready jobs are waiting,
    // else this would cause an endless loop. Abort with an error if so.
    if (
            (
            this->localRunningJobs.Size() +
            this->remoteRunningJobs.Size() + 
            this->initializeJobs.Size()
            ) == 0 &&
        this->readyJobs.Size() > 0
        )
    {
        Console::Instance()->Error("There was no valid service found for processing jobs.\n");
    }

    this->isRunningJobs = true;   
}
//------------------------------------------------------------------------------
/**
    Adds ready jobs to local job list	
*/
void
DistributedJobScheduler::StartReadyJobsLocal()
{  
    IndexT i = 0;
    while (i<this->readyJobs.Size() && this->initializeJobs.Size()==0)
    {
        // add local job
        bool addJob = false;
        bool skipUpdate = this->isRunningJobs;
        if(this->localRunningJobs.Size() < this->maxParallelLocalJob)
        {
            addJob = true;
            if (this->localRunningJobs.Size()>0)
            {
                // if a local job is already in the job list, skip the update process
                // of every new job...
                skipUpdate = true;
                // only add jobs to the same list, if they need the same revision
                // and work on the same project path
                if(
                    this->localRunningJobs[0]->GetRequiredRevision()!=this->readyJobs[i]->GetRequiredRevision() &&
                    this->localRunningJobs[0]->GetProjectDirectory()!=this->readyJobs[i]->GetProjectDirectory()
                    )
                {
                    addJob = false;
                }
            }
            if(DistributedJob::Ready != this->readyJobs[i]->GetCurrentState())
            {
                addJob = false;
            }
        }
        if (addJob)
        {            
            n_printf("[Local job #%s] STARTING\n",this->readyJobs[i]->GetIdentifier().AsCharPtr());
            this->localRunningJobs.Append(this->readyJobs[i]);
            // validate the project path of the job
            this->readyJobs[i]->ValidateProjectPath();
            if(skipUpdate)
            {
                this->readyJobs[i]->SkipProjectDirectoryUpdate();
            }
            this->readyJobs.EraseIndex(i);
        }
        else
        {
            i++;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Adds ready jobs to remote job lists	
*/
void
DistributedJobScheduler::StartReadyJobsRemote()
{
    SizeT addedJobCount;
    // check all services
    IndexT s;
    for (s = 0; s < this->remoteServices.Size(); s++)
    {
        // skip service if it was banned
        if (this->bannedRemoteServices.FindIndex(this->remoteServices[s]) == InvalidIndex)
        {
            addedJobCount = 0;
            IndexT i = 0;
            // while there are ready jobs and free processors on the service
            while ( i < this->readyJobs.Size() && addedJobCount < this->remoteServices[s]->GetProcessorCount())
            {
                // becomes true if current service is able to accept jobs
                bool addJob = false;
	            // is service connected?
                if (RemoteJobService::Closed == this->remoteServices[s]->GetServiceState())
                {
                    this->remoteServices[s]->Open();
                    // wait until the service is ready to accept jobs or connection failed
                    while (RemoteJobService::Connecting == this->remoteServices[s]->GetServiceState())
                    {
                        this->remoteServices[s]->Update();
                    }
                }
                // Is the service available for this master
                if(
                    RemoteJobService::Ready == this->remoteServices[s]->GetServiceState() ||
                    RemoteJobService::ExclusiveReady == this->remoteServices[s]->GetServiceState())
                {
                    addJob = true;
                }
                else
                {
                    // this service seems to be unable to process any job, skip it...
                    break;
                }
                
                if(addJob)
                {
                    // send the job to the service. If it failed, skip the service...
                    if(this->remoteServices[s]->SendJob(this->readyJobs[i]))
                    {
                        n_printf("[Remote job #%s] STARTING\n",this->readyJobs[i]->GetIdentifier().AsCharPtr());
                        this->remoteRunningJobs.Append(this->readyJobs[i]);
                        this->readyJobs.EraseIndex(i);
                        addedJobCount++;
                    }
                    else
                    {
                        i++;
                    }
                }
                else
                {
                    // skip service if it wasn't able to accept jobs
                    i++;
                }   
            }
            
            // start the service, if any job was attached
            if (
                    (
                    RemoteJobService::Ready == this->remoteServices[s]->GetServiceState() ||
                    RemoteJobService::ExclusiveReady == this->remoteServices[s]->GetServiceState()
                    ) &&
                addedJobCount > 0
                )
            {
                // create clones of the initialize/finalize job lists
                Util::Array<Ptr<DistributedJob>> initJobs;
                Util::Array<Ptr<DistributedJob>> finaJobs;
                IndexT idx;
                for(idx = 0; idx < this->remoteInitializeJobs.Size(); idx++)
                {
            	    initJobs.Append(this->remoteInitializeJobs[idx]->Clone());
                }
                for(idx = 0; idx < this->remoteFinalizeJobs.Size(); idx++)
                {
                    finaJobs.Append(this->remoteFinalizeJobs[idx]->Clone());
                }
                // add the initialize/finalize jobs to the remote running job list
                this->remoteRunningJobs.AppendArray(initJobs);
                this->remoteRunningJobs.AppendArray(finaJobs);
                // add the initialize/finalize jobs to the remote service
                this->remoteServices[s]->SetInitializeJobs(initJobs);
                this->remoteServices[s]->SetFinalizeJobs(finaJobs);
                // tell the service to run all its jobs
                this->remoteServices[s]->RunJobs();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Process a local job. Depending on the job state, this will do
    different things.
*/
void
DistributedJobScheduler::ProcessLocalJob(const Ptr<DistributedJob> & job)
{
    if(DistributedJob::Ready == job->GetCurrentState())
    {
        job->ValidateProjectPath();
    }
    else if(DistributedJob::ValidProjectPath == job->GetCurrentState())
    {
        // Check if other local jobs are doing an svn update
        // or are working on the project directory  at the moment.
        // If not, then start the svn update of the job
        bool isAnyUpdating = false;
        IndexT i;
        for (i = 0; i<this->localRunningJobs.Size(); i++)
        {
            if(
                DistributedJob::Updating == this->localRunningJobs[i]->GetCurrentState() ||
                DistributedJob::Working == this->localRunningJobs[i]->GetCurrentState()
                )
            {
                isAnyUpdating = true;
                break;
            }
        }
        if(!isAnyUpdating)
        {
            job->UpdateProjectDirectory();
        }
    }
    else if(DistributedJob::Updating == job->GetCurrentState())
    {
        job->Update();
    }
    else if(DistributedJob::UpToDate == job->GetCurrentState())
    {
        // Check if other local jobs are doing an svn update
        // If not, then start the job
        bool isAnyUpdating = false;
        IndexT i;
        for (i = 0; i<this->localRunningJobs.Size(); i++)
        {
            if(DistributedJob::Updating == this->localRunningJobs[i]->GetCurrentState())
            {
                isAnyUpdating = true;
                break;
            }
        }
        if(!isAnyUpdating)
        {
            job->RunJob();
        }
    }
    else if(DistributedJob::Working == job->GetCurrentState())
    {
        job->Update();
    }
}

//------------------------------------------------------------------------------
/**
    Returns true if any remote service is still busy
*/
bool
DistributedJobScheduler::HasRunningRemoteServices()
{
    bool found = false;
    IndexT si;
    for(si = 0; si < this->remoteServices.Size(); si++)
    {
        if (this->bannedRemoteServices.FindIndex(this->remoteServices[si]) == InvalidIndex)
        {
            found |= (this->remoteServices[si]->GetServiceState() == RemoteJobService::Busy);
        }
    }
    return found;
}

//------------------------------------------------------------------------------
/**
    Send the service to the blacklist and handle its currently running
    jobs. 
*/
void
DistributedJobScheduler::DisableRunningService(const Ptr<RemoteJobService> & service)
{
    // check if service is already banned
    n_assert(this->bannedRemoteServices.FindIndex(service)==InvalidIndex);
    
    // append service to blacklist
    this->bannedRemoteServices.Append(service);
    
    // reset jobs from service
    this->ResetJobsFromService(service);
    
    // reset the service and mark it as dirty
    service->SetDirty();
    service->Reset();
}

//------------------------------------------------------------------------------
/**
    Sends a message to the given service and checks for it's current state.
    If the service reported, that it has cleaned itself up, then remove it from
    the banned service list.
*/
void
DistributedJobScheduler::HandleBannedService(const Ptr<RemoteJobService> & service)
{
    // check if the service reported, that is has cleaned up.
    if (service->HasCleanedUp())
    {
        this->bannedRemoteServices.EraseIndex(this->bannedRemoteServices.FindIndex(service));
    }
    else
    {
        if (service->GetServiceState() == RemoteJobService::Closed)
        {
            service->Open();
            // wait until the service is ready to accept commands or connection failed
            while (RemoteJobService::Connecting == service->GetServiceState())
            {
                service->Update();
            }
        }
        if(service->IsOpen())
        {
            service->Update();
            if (
                RemoteJobService::Ready == service->GetServiceState() ||
                RemoteJobService::ExclusiveReady == service->GetServiceState())
            {
                // send "you was banned" to the service.
                service->SendBannedInfo();
            }
        }            
    }
}

//------------------------------------------------------------------------------
/**
	Catch all jobs of a service and reappend them to the ready-job list
*/
void
DistributedJobScheduler::ResetJobsFromService(const Ptr<RemoteJobService> & service)
{
    // remove all initialize, finalize and standard jobs from remote job list
    Array<Ptr<DistributedJob>> removeJobs;
    removeJobs.AppendArray(service->GetUncompleteJobs());
    removeJobs.AppendArray(service->GetInitializeJobs());
    removeJobs.AppendArray(service->GetFinalizeJobs());
    IndexT remIdx;
    IndexT foundIndex;
    for (remIdx = 0; remIdx < removeJobs.Size(); remIdx++)
    {
        foundIndex = this->remoteRunningJobs.FindIndex(removeJobs[remIdx]);
        if(foundIndex != InvalidIndex)
        {
            this->remoteRunningJobs.EraseIndex(foundIndex);
        }
    }

    // get all job guids sent to this service and re-assign them to the
    // ready job list.
    Console::Instance()->Print("Re-Append %i jobs...\n",service->GetUncompleteJobs().Size());
    this->readyJobs.AppendArray(service->GetUncompleteJobs());
}

} // namespace DistributedTools