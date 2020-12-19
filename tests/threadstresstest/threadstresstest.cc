//------------------------------------------------------------------------------
// threadstresstest.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "threadstresstest.h"
#include "io/ioserver.h"
#include "app/application.h"
#include "threading/thread.h"
#include "threading/safequeue.h"
#include "threading/safepriorityqueue.h"

using namespace Timing;
using namespace Threading;
namespace Test
{
__ImplementClass(ThreadStressTest, 'STTE', Core::RefCounted);

thread_local static uint32_t state = 4711;
uint32_t XorShift()
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

Threading::SafeQueue<uint32_t> queue;
Threading::SafeQueue<ThreadId> stoppedQueue;


class WorkerThread : public Thread
{
    __DeclareClass(WorkerThread);
public:
    void DoWork()
    {
        while (loops--)
        {
            uint32_t items = XorShift() % 126;
                        
            for (uint32_t i = 0; i < items; i++)
            {                
                queue.Enqueue(Thread::GetMyThreadId());
                this->YieldThread();
            }           
        }
        stoppedQueue.Enqueue(Thread::GetMyThreadId());

    }
    uint32_t loops = 50000;
};

__ImplementClass(WorkerThread, 'WTTR', Threading::Thread);


class ReaderThread : public Thread
{
    __DeclareClass(ReaderThread);
public:
    void DoWork()
    {
        while (!this->ThreadStopRequested())
        {                
            if(!queue.IsEmpty())
            {
                queue.Dequeue();                             
            }            
        }    
        while(!queue.IsEmpty())
        {
            queue.Dequeue();
        }
    }    
};

__ImplementClass(ReaderThread, 'RDTR', Threading::Thread);



//------------------------------------------------------------------------------
/**
*/
void
ThreadStressTest::Run()
{
    App::Application app;

    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

    app.SetAppTitle("Jobs test!");
    app.SetCompanyName("gscept");
    app.Open();

    Util::Array<Ptr<WorkerThread>> workers;
    for (int i = 0; i < 20; i++)
    {
        Ptr<WorkerThread> worker = WorkerThread::Create();
        workers.Append(worker);
        Util::String name;
        name.Format("Writer%d", i);
        worker->SetName(name);
        worker->Start();
    }
    Ptr<ReaderThread> reader = ReaderThread::Create();
    reader->SetName("Reader");
    reader->Start();
    
    while (stoppedQueue.Size() < 10)
    {
        n_sleep(0.1);
    }
    reader->Stop();
    while (reader->IsRunning())
    {
        n_sleep(0.1);
    }

    app.Close();
}

} // namespace Test