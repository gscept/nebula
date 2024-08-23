#pragma once
//------------------------------------------------------------------------------
/**
    The resource loader thread is responsible to handle all ResourceLoaders that wish to load resources asynchronously
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "threading/thread.h"
#include "threading/safequeue.h"
#include <functional>
#include "resourceid.h"

namespace IO
{
class IoServer;
}

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

protected:
    /// Runs before jobs are getting picked off the list
    virtual void OnBeforeJobs();
    /// Runs after jobs are finished for this iteration
    virtual void OnAfterJobs();

private:
    friend class ResourceLoader;

    /// perform work
    void DoWork() override;
    /// emit wakeup signal
    virtual void EmitWakeupSignal() override;
    
    Threading::SafeQueue<std::function<void()>> jobs;
    Threading::Event completeEvent;
    Ptr<IO::IoServer> ioServer;
};
} // namespace Resources
