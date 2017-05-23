#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPWorkerThread
  
    The worker thread class of the thread-pool job system.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "threading/thread.h"
#include "threading/safequeue.h"
#include "jobs/tp/tpjobcommand.h"     
#include "debug/debugtimer.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPWorkerThread : public Threading::Thread
{
    __DeclareClass(TPWorkerThread);
public:
    /// constructor
    TPWorkerThread();
    /// destructor
    virtual ~TPWorkerThread();
    
    /// called if thread needs a wakeup call before stopping
    virtual void EmitWakeupSignal();
    /// this method runs in the thread context
    virtual void DoWork();      
    /// request threading code to stop, returns when thread has actually finished
    void Stop();

    /// push a job command onto the job queue
    void PushJobCommand(const TPJobCommand& cmd);

private:
    /// process a single job slice
    void ProcessJobSlices(TPJobSlice* firstSlice, ushort numSlices, ushort stride);

    static const SizeT MaxScratchSize = (64 * 1024);    // 64 kB max scratch size

    Threading::SafeQueue<TPJobCommand> jobQueue;
    ubyte* scratchBuffer;
      
#if NEBULA3_ENABLE_PROFILING
    _declare_timer(debugTimer);
#endif
};

} // namespace Jobs
//------------------------------------------------------------------------------
