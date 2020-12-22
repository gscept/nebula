//------------------------------------------------------------------------------
// resourceloaderthread.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/ioserver.h"
#include "resourceloaderthread.h"
#include "profiling/profiling.h"

namespace Resources
{

__ImplementClass(Resources::ResourceLoaderThread, 'RETH', Threading::Thread);
//------------------------------------------------------------------------------
/**
*/
ResourceLoaderThread::ResourceLoaderThread() :
    completeEvent(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoaderThread::~ResourceLoaderThread()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoaderThread::DoWork()
{
    this->ioServer = IO::IoServer::Create();
    Profiling::ProfilingRegisterThread();
    Util::Array<std::function<void()>> arr;
    arr.Reserve(1000);
    while (!this->ThreadStopRequested())
    {
        this->completeEvent.Reset();

        this->jobs.DequeueAll(arr);
        IndexT i;
        for (i = 0; i < arr.Size(); i++)
        {
            arr[i]();
        }
        arr.Reset();

        // signal that this batch is complete
        this->completeEvent.Signal();

        // wait for more jobs!
        this->jobs.Wait();
    }

    this->ioServer = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoaderThread::EmitWakeupSignal()
{
    this->jobs.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoaderThread::Wait()
{
    this->completeEvent.Wait();
}

} // namespace Resources
