//------------------------------------------------------------------------------
//  tpjobport.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/tp/tpjobport.h"
#include "jobs/jobsystem.h"

namespace Jobs
{
__ImplementClass(Jobs::TPJobPort, 'TPJP', Base::JobPortBase);

using namespace Base;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
TPJobPort::TPJobPort() :
	lastPushedJob(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TPJobPort::~TPJobPort()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobPort::Discard()
{
    n_assert(this->IsValid());
    this->lastPushedJob = 0;
    JobPortBase::Discard();
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobPort::PushJob(const Ptr<Job>& job)
{
    this->lastPushedJob = job;
    job->NotifyStart();
    const FixedArray<TPJobSlice>& jobSlices = job->GetJobSlices();
    JobSystem::Instance()->GetThreadPool()->PushJobSlices(&(jobSlices[0]), jobSlices.Size());
}

//------------------------------------------------------------------------------
/**
    Push a job chain, where each job in the chain depends on the previous
    job. This will also guarantee, that the first job slice of each
    job will run on the same worker thread. In case of simple jobs
    (with only one slice) this improves load.
*/
void
TPJobPort::PushJobChain(const Array<Ptr<Job> >& jobs)
{    
    n_assert(!jobs.IsEmpty());
    TPJobThreadPool* threadPool = JobSystem::Instance()->GetThreadPool();
    IndexT threadIndex = threadPool->GetNextThreadIndex();
    IndexT i;
    for (i = 0; i < jobs.Size(); i++)
    {
        // push a sync between jobs
        if (i > 0)
        {
            JobSystem::Instance()->GetThreadPool()->PushSync(jobs[i - 1]->GetCompletionEvent());
        }

        // push the job slices
        jobs[i]->NotifyStart();
        const FixedArray<TPJobSlice>& jobSlices = jobs[i]->GetJobSlices();
        JobSystem::Instance()->GetThreadPool()->PushJobSlices(&(jobSlices[0]), jobSlices.Size(), threadIndex);
    }
    this->lastPushedJob = jobs.Back();
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobPort::PushFlush()
{
    // this is used on the PS3 implementation to notify the port
    // that uniform data has changed, has no effect in the
    // thread-pool job system.
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobPort::PushSync()
{
    if (this->lastPushedJob.isvalid())
    {
        JobSystem::Instance()->GetThreadPool()->PushSync(this->lastPushedJob->GetCompletionEvent());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobPort::WaitDone()
{
    n_assert(this->lastPushedJob.isvalid());
    this->lastPushedJob->GetCompletionEvent()->Wait();
}

//------------------------------------------------------------------------------
/**
*/
bool
TPJobPort::CheckDone()
{
    if (this->lastPushedJob.isvalid())
    {
        return this->lastPushedJob->GetCompletionEvent()->Peek();
    }
    return true;
}

} // namespace Jobs