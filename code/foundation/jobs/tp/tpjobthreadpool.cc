//------------------------------------------------------------------------------
//  tpjobthreadpool.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/tp/tpjobthreadpool.h"
#include "system/systeminfo.h"

namespace Jobs
{
using namespace Util;
using namespace System;
using namespace Threading;

//------------------------------------------------------------------------------
/**
*/
TPJobThreadPool::TPJobThreadPool() :
    isValid(false),
    nextThreadIndex(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TPJobThreadPool::~TPJobThreadPool()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobThreadPool::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;

    // setup worker threads
    // FIXME: on Win32 platforms handle distribution of threads to
    // cores a bit more clever...
    String threadName;
    IndexT i;
    for (i = 0; i < NumWorkerThreads; i++)
    {
        threadName.Format("JobWorker%d", i);
        this->workerThreads[i] = TPWorkerThread::Create();
        this->workerThreads[i]->SetPriority(Thread::High);
        this->workerThreads[i]->SetCoreId(Cpu::JobThreadFirstCore + i);
        this->workerThreads[i]->SetName(threadName);
        this->workerThreads[i]->Start();
    }
    this->nextThreadIndex = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobThreadPool::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
    IndexT i;
    for (i = 0; i < NumWorkerThreads; i++)
    {
        this->workerThreads[i]->Stop();
        this->workerThreads[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobThreadPool::PushJobSlices(TPJobSlice* firstSlice, SizeT numSlices, IndexT threadIndex)
{
    n_assert(0 != firstSlice);
    n_assert(numSlices > 0);

    // override start thread index if defined
    if (InvalidIndex != threadIndex)
    {
        this->nextThreadIndex = threadIndex;
    }

    // split the job slices into NumWorkerThreads chunks
    IndexT i;
    ushort numWorkUnitSlices[NumWorkerThreads];
    for (i = 0; i < NumWorkerThreads; i++)
    {
        numWorkUnitSlices[i] = ushort(numSlices / NumWorkerThreads);
    }
    SizeT remainder = numSlices % NumWorkerThreads;
    ushort stride = NumWorkerThreads;
    for (i = 0; i < remainder; i++)
    {
        numWorkUnitSlices[i] += 1;
    }
    for (i = 0; i < NumWorkerThreads; i++)
    {
        if (numWorkUnitSlices[i] > 0)
        {
            TPJobCommand jobCmd;
            jobCmd.SetupRun(firstSlice + i, numWorkUnitSlices[i], stride);
            this->workerThreads[nextThreadIndex++]->PushJobCommand(jobCmd);
            if (this->nextThreadIndex >= NumWorkerThreads)
            {
                this->nextThreadIndex = 0;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobThreadPool::PushSync(const Threading::Event* syncEvent)
{
    TPJobCommand jobCmd;
    jobCmd.SetupSync(syncEvent);
    IndexT i;
    for (i = 0; i < NumWorkerThreads; i++)
    {
        this->workerThreads[i]->PushJobCommand(jobCmd);
    }
}

} // namespace Jobs
