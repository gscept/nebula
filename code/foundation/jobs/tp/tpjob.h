#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPJob
  
    Job implementation for the thread-pool job system.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "jobs/base/jobbase.h"
#include "jobs/tp/tpjobslice.h"
#include "util/fixedarray.h"
#include "threading/event.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPJob : public Base::JobBase
{
    __DeclareClass(TPJob);
public:
    /// constructor
    TPJob();
    /// destructor
    ~TPJob();

    /// setup the job
    void Setup(const Jobs::JobUniformDesc& uniformDesc, const Jobs::JobDataDesc& inputDesc, const Jobs::JobDataDesc& outputDesc, const Jobs::JobFuncDesc& funcDesc);
    /// discard the job
    void Discard();

private:
    friend class TPWorkerThread;
    friend class TPJobPort;

    /// notify the job object that it is going to be started (called by TPJobPort)
    void NotifyStart();
    /// signal completion of N slices, called by TPWorkerThread!
    void NotifySlicesComplete(ushort numSlices);
    /// get job slices
    const Util::FixedArray<TPJobSlice>& GetJobSlices() const;
    /// get pointer to the completion event
    const Threading::Event* GetCompletionEvent() const;

    Util::FixedArray<TPJobSlice> jobSlices;
    volatile int completionCounter;
    Threading::Event completionEvent;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::FixedArray<TPJobSlice>&
TPJob::GetJobSlices() const
{
    return this->jobSlices;
}

//------------------------------------------------------------------------------
/**
*/
inline const Threading::Event*
TPJob::GetCompletionEvent() const
{
    return &this->completionEvent;
}

//------------------------------------------------------------------------------
/**
    Notify the job that it is going to be started.
*/
inline void
TPJob::NotifyStart()
{
    n_assert(0 == this->completionCounter);
    Threading::Interlocked::Exchange(&this->completionCounter, this->jobSlices.Size());
    this->completionEvent.Reset();
}

//------------------------------------------------------------------------------
/**
    This method will be called from a worker thread(!) when it has finished
    processing a slice.
    Also, due to the nature of the thread-pool system, this method will
    not be called very frequently (not after every job slice, but only
    max 4 times per job (number of parallel worker threads).
*/
inline void
TPJob::NotifySlicesComplete(ushort numSlices)
{
    int modify = -numSlices;
	Threading::Interlocked::Add(this->completionCounter, modify);
	n_assert(this->completionCounter >= 0);
    if (this->completionCounter == 0)
    {
        this->completionEvent.Signal();
    }
}

} // namespace Jobs
//------------------------------------------------------------------------------
