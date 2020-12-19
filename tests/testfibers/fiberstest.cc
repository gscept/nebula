//------------------------------------------------------------------------------
// visibilitytest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "fiberstest.h"
#include "app/application.h"
#include "io/ioserver.h"
#include "threading/lockfreequeue.h"

#include "fibers/fibers.h"

using namespace Timing;
using namespace Fibers;
namespace Test
{

Threading::LockFreeQueue<int> TestQueue;

void 
F2(void* context)
{
    int* ctx = (int*)context;

    TestQueue.Enqueue(*ctx);
    AtomicCounter counter;
    Util::FixedArray<int*> contexts(100);
    contexts.Fill(ctx);
    Enqueue([](void* ctx) 
        {
            int* val = (int*)ctx;
            Threading::Interlocked::Add(*val, 1);
            //TestQueue.Enqueue(*val);
        }, contexts, &counter);

    Wait(&counter, 0);

    // remove 100 items from the queue
    int dummy;
    for (IndexT i = 0; i < contexts.Size(); i++)
        TestQueue.Dequeue(dummy);

}

void 
F1(void* context)
{
    AtomicCounter counter;

    int* ctx = (int*)context;
    Util::FixedArray<int*> contexts(1);
    contexts.Fill(ctx);
    Enqueue(F2, contexts, &counter);
    Wait(&counter, 0);

    int dummy;
    TestQueue.Dequeue(dummy);
}


__ImplementClass(FibersTest, 'FITE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
void
FibersTest::Run()
{
    App::Application app;

    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

    app.SetAppTitle("Fibers test!");
    app.SetCompanyName("gscept");
    app.Open();

    TestQueue.Resize(1024);

    FiberQueueCreateInfo info;
    info.numFibers = 256;
    info.numThreads = 8;
    FiberQueue::Setup(info);

    Timing::Timer timer;
    int context = 0;
    Util::FixedArray<int*> contexts(100);
    contexts.Fill(&context);
    AtomicCounter counter;
    timer.Start();
    Enqueue(F1, contexts, &counter);

    Lock(&counter, 0);

    VERIFY(TestQueue.Size() == 0);
    timer.Stop();
    printf("Fibers took %f seconds\n", timer.GetTime());
    VERIFY(context == 10000);

    FiberQueue::Discard();

    app.Close();
}

} // namespace Test