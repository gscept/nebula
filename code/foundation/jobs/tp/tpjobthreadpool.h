#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPJobThreadPool
    
    Manages the thread-pool, distributes TPJobSlice objects to the
    worker threads.

    FIXME: class is currently not thread-safe!
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "jobs/tp/tpworkerthread.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPJobThreadPool
{
public:
    /// constructor
    TPJobThreadPool();
    /// destructor
    ~TPJobThreadPool();
    
    /// setup the thread pool
    void Setup();
    /// discard the thread pool
    void Discard();
    /// return true if object is setup
    bool IsValid() const;

    /// push a sync command into the thread pool
    void PushSync(const Threading::Event* syncEvent);
    /// push job slices into the the thread pool
    void PushJobSlices(TPJobSlice* firstSlice, SizeT numSlices, IndexT threadIndex=InvalidIndex);
    /// get a suitable worker thread index (optimally a thread with currently no load)
    IndexT GetNextThreadIndex() const;

private:
    static const SizeT NumWorkerThreads = 8;
    Threading::CriticalSection critSect;
    Ptr<TPWorkerThread> workerThreads[NumWorkerThreads];
    Util::Array<TPJobSlice*> tmpJobSliceQueue[NumWorkerThreads];
    IndexT nextThreadIndex;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
TPJobThreadPool::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
TPJobThreadPool::GetNextThreadIndex() const
{
    return this->nextThreadIndex;
}

} // namespace Jobs
//------------------------------------------------------------------------------
    