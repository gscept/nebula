#pragma once
//------------------------------------------------------------------------------
/**
    Job system allows for scheduling and execution of a parallel task.

    The job system works as follows. First one creates a job port with a name
    and some threads. The threads will be named as the port but with their index appended.

    The job port has a priority, meaning all jobs sent to this port will inherit said priority.
    The job ports are then updated every frame to schedule work packages.

    A job is not single-threaded, but spreads its work by chunking its slices and scheduling
    an equal amount of work on several threads. If you require the jobs to execute in sequence,
    you can execute a sequence of jobs which will be guaranteed to run on the same thread.

    To synchronize, you have to create a job synchronization primitive, and it allows
    for signaling, waiting on the host-side, and waiting on the threads between jobs.

    How to setup a job:
        Create port, create a job when required, use the function context to provide the
        job with inputs, outputs and uniform data.
        

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idallocator.h"
#include "threading/thread.h"
#include "threading/event.h"
#include "threading/safequeue.h"
#include "util/stringatom.h"
#include "util/queue.h"
#include <atomic>
namespace Jobs
{

// these values could be bigger if we need it
#define JOBFUNCCONTEXT_MAXIO 8
#define JOBFUNCCONTEXT_MAXUNIFORMS 4

#define N_JOB_INPUT(ctx, slice, index) (ctx.inputs[index] + slice * ctx.inputSizes[index])
#define N_JOB_OUTPUT(ctx, slice, index) (ctx.outputs[index] + slice * ctx.outputSizes[index])

//------------------------------------------------------------------------------
/**
    This class describes the size of a workload for a single thread (can be multiple items)
*/
struct JobFuncContext
{
    ubyte* scratch;

    uint numUniforms;                               // number of used slots (max 4)
    ubyte* uniforms[JOBFUNCCONTEXT_MAXUNIFORMS];
    uint uniformSizes[JOBFUNCCONTEXT_MAXUNIFORMS];

    uint numInputs;                                 // number of used slots (max 4)
    ubyte* inputs[JOBFUNCCONTEXT_MAXIO];
    uint inputSizes[JOBFUNCCONTEXT_MAXIO];

    uint numOutputs;                                // number of used slots (max 4)
    ubyte* outputs[JOBFUNCCONTEXT_MAXIO];
    uint outputSizes[JOBFUNCCONTEXT_MAXIO];

    uint numSlices;
};

struct JobIOData
{
    static const SizeT MaxNumBuffers = JOBFUNCCONTEXT_MAXIO;

    SizeT numBuffers;
    void* data[MaxNumBuffers];          // pointers to data
    SizeT dataSize[MaxNumBuffers];      // size of entire array
    SizeT sliceSize[MaxNumBuffers];     // size of singular work package
};

struct JobUniformData
{
    static const SizeT MaxNumBuffers = JOBFUNCCONTEXT_MAXUNIFORMS;

    SizeT numBuffers;
    const void* data[MaxNumBuffers];    // pointers to data
    SizeT dataSize[MaxNumBuffers];      // size of entire array
    SizeT scratchSize;                  // scratch memory
};

struct JobContext
{
    JobIOData input;
    JobIOData output;
    JobUniformData uniform;
};

class JobThread : public Threading::Thread
{
    __DeclareClass(JobThread);
public:

    enum JobThreadCommandType
    {
        RunJob
        , Signal
        , Wait
        , WaitAndReset
        
    };

    struct JobThreadCommand
    {
        JobThreadCommandType ev;
        union
        {
            struct // execute job
            {
                uint slice;
                uint numSlices;
                JobContext context;
                void(*JobFunc)(const JobFuncContext& ctx);
                const std::function<void()>* callback;
            } run;

            struct // synchronize
            {
                Threading::Event* ev;
                std::atomic_uint* completionCounter;
                const std::function<void()>* callback;
            } sync;
        };
    };

    /// constructor
    JobThread();
    /// destructor
    virtual ~JobThread();

    /// called if thread needs a wakeup call before stopping
    void EmitWakeupSignal();
    /// this method runs in the thread context
    void DoWork();
    /// returns true if thread has work
    bool HasWork();

    /// push a set of job slices
    void RunJobSlices(uint sliceIndex, uint numSlices, const JobContext ctx, void(*JobFunc)(const JobFuncContext& ctx), const std::function<void()>* callback);
    /// push command buffer work
    void PushCommand(const JobThreadCommand& command);
    /// push command buffer work
    void PushCommands(const Util::Array<JobThreadCommand>& commands);

private:

    static const SizeT MaxScratchSize = (64 * 1024);    // 64 kB max scratch size

    Threading::SafeQueue<JobThreadCommand> commands;
    ubyte* scratchBuffer;
};

//------------------------------------------------------------------------------

ID_32_TYPE(JobId);
ID_16_TYPE(JobPortId);
ID_32_TYPE(JobSyncId);

struct CreateJobPortInfo
{
    Util::StringAtom name;
    SizeT numThreads;
    uint affinity;
    uint priority;
};

/// create a new job port
JobPortId CreateJobPort(const CreateJobPortInfo& info);
/// destroy job port
void DestroyJobPort(const JobPortId& id);

/// check to see if port is idle
bool JobPortBusy(const JobPortId& id);

enum
{
    JobPort_Name,
    JobPort_Threads,
    JobPort_NextThreadIndex,
};

typedef Ids::IdAllocator<
    Util::StringAtom,                       // 0 - name
    Util::FixedArray<Ptr<JobThread>>,       // 1 - threads
    uint                                    // 2 - next thread index
> JobPortAllocator;
extern JobPortAllocator jobPortAllocator;

//------------------------------------------------------------------------------

struct CreateJobInfo
{
    void(*JobFunc)(const JobFuncContext& ctx);
};

/// create job
JobId CreateJob(const CreateJobInfo& info);
/// delete job
void DestroyJob(const JobId& id);

/// schedule job to be executed
void JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx, const bool cycleThreads = true);
/// schedule job with callback when finished
void JobSchedule(const JobId& job, const JobPortId& port, const JobContext& ctx, const std::function<void()>& callback, const bool cycleThreads = true);
/// schedule job without a context
void JobSchedule(const JobId& job, const JobPortId& port);
/// schedule a job with a poiunter as context and a work group size and item count
void JobSchedule(const JobId& job, const JobPortId& port, void* ctx, SizeT count, SizeT groupSize, const bool cycleThreads = true);
/// schedule a sequence of jobs
void JobScheduleSequence(const Util::Array<JobId>& jobs, const JobPortId& port, const Util::Array<JobContext>& contexts);
/// schedule a sequence of jobs
void JobScheduleSequence(const Util::Array<JobId>& jobs, const JobPortId& port, const Util::Array<JobContext>& contexts, const std::function<void()>& callback);
/// allocate memory for job
void* JobAllocateScratchMemory(const JobId& job, const Memory::HeapType heap, const SizeT size);

struct PrivateMemory
{
    Memory::HeapType heapType;
    SizeT size;
    void* memory;
};

enum
{
    Job_CreateInfo,
    Job_CallbackFunc,
    Job_ScratchMemory
};

typedef Ids::IdAllocator<
    CreateJobInfo,              // 0 - job info
    std::function<void()>,      // 1 - callback
    PrivateMemory               // 4 - private buffer, destroyed when job is finished
> JobAllocator;
extern JobAllocator jobAllocator;


//------------------------------------------------------------------------------

struct CreateJobSyncInfo
{
    std::function<void()> callback;
};

enum
{
    SyncCallback,
    SyncCompletionEvent,
    SyncCompletionCounter,
    SyncPendingSignal
};

/// create job sync
JobSyncId CreateJobSync(const CreateJobSyncInfo& info);
/// destroy job sync
void DestroyJobSync(const JobSyncId id);

/// reset job sync on host
void JobSyncHostReset(const JobSyncId id);
/// signal job sync on host
void JobSyncHostSignal(const JobSyncId id, bool reset = true);
/// put job sync on port, if reset is true, reset prior to signaling
void JobSyncThreadSignal(const JobSyncId id, const JobPortId port, bool reset = true);
/// wait for job on host side, if reset is true, resets after waiting
void JobSyncHostWait(const JobSyncId id, bool reset = false);
/// wait for job on thread side, if reset is true, reset after waiting
void JobSyncThreadWait(const JobSyncId id, const JobPortId port, bool reset = false);

/// returns true if sync object has been signaled
bool JobSyncSignaled(const JobSyncId id);

typedef Ids::IdAllocator<
    std::function<void()>,      // 0 - callback
    Threading::Event*,          // 1 - event
    std::atomic_uint*,          // 2 - completion counter
    bool                        // 3 - pending signal
> JobSyncAllocator;
extern JobSyncAllocator jobSyncAllocator;

} // namespace Jobs
