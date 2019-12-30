//------------------------------------------------------------------------------
// resourceloaderthread.cc
// (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "resourceloaderthread.h"

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