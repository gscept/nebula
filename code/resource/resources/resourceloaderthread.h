#pragma once
//------------------------------------------------------------------------------
/**
	The resource loader thread is responsible to handle all ResourceLoaders that wish to load resources asynchronously
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "threading/thread.h"
#include "threading/safequeue.h"
#include <functional>
#include "resourceid.h"
namespace Resources
{
class ResourceLoaderThread : public Threading::Thread
{
	__DeclareClass(ResourceLoaderThread);
public:
	/// constructor
	ResourceLoaderThread();
	/// destructor
	virtual ~ResourceLoaderThread();

	/// wait for the thread to be done (must be called from outside this thread!)
	void Wait();

private:
	friend class ResourceStreamPool;

	/// perform work
	void DoWork();
	/// emit wakeup signal
	void EmitWakeupSignal();

	Threading::SafeQueue<std::function<void()>> jobs;
	Threading::Event completeEvent;
};
} // namespace Resources