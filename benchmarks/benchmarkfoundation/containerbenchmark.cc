//------------------------------------------------------------------------------
//  containerbenchmark.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "containerbenchmark.h"
#include "util/queue.h"
#include "util/dequeue.h"
#include "util/list.h"


const int numObjects = 100000;

#define DECLARE_CB_TEST(name, ctype, dtype) ContainerBenchmark<ctype<dtype>, dtype> name;
#define SETUP_CB_TEST(cb, data, addfunc, rmfunc ) cb.Setup(data, [](decltype(cb)::ctype& c, decltype(cb)::dtype d) { c.addfunc(d); },  [&](decltype(cb)::ctype& c) { return c.rmfunc(); } );
#define RUN_CB_TEST(cb, timer) timer.Start(); cb.Run(timer, numObjects);timer.Stop();


namespace Benchmarking
{
__ImplementClass(Benchmarking::ContainerBench, 'CTEB', Benchmarking::Benchmark);

using namespace Core;
using namespace Timing;



//------------------------------------------------------------------------------
/**
*/
void
ContainerBench::Run(Timer& timer)
{
     
    DECLARE_CB_TEST(cb, Util::Queue, int);
    SETUP_CB_TEST(cb, 1024, Enqueue, Dequeue);
    DECLARE_CB_TEST(cbs, Util::Queue, Util::String);
    SETUP_CB_TEST(cbs, "test123", Enqueue, Dequeue);

    DECLARE_CB_TEST(cbq, Util::DeQueue, int);
    SETUP_CB_TEST(cbq, 1024, Enqueue, Dequeue);
    DECLARE_CB_TEST(cbqs, Util::DeQueue, Util::String);
    SETUP_CB_TEST(cbqs, "test123", Enqueue, Dequeue);

    DECLARE_CB_TEST(cbl, Util::List, int);
    SETUP_CB_TEST(cbl, 1024, AddFront, RemoveBack);
    DECLARE_CB_TEST(cbls, Util::List, Util::String);
    SETUP_CB_TEST(cbls, "test123", AddFront, RemoveBack);
    
    RUN_CB_TEST(cb, timer);
    RUN_CB_TEST(cbq, timer);
    RUN_CB_TEST(cbl, timer);


    RUN_CB_TEST(cbs, timer);    
    RUN_CB_TEST(cbqs, timer);
    RUN_CB_TEST(cbls, timer);
    
}

} // namespace Benchmarking
