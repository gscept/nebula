//------------------------------------------------------------------------------
//  mempoolbenchmark.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "mempoolbenchmark.h"
#include "memory/memorypool.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::MemPoolBenchmark, 'MPBM', Benchmarking::Benchmark);

using namespace Timing;
using namespace Memory;

//------------------------------------------------------------------------------
/**
*/
void
MemPoolBenchmark::Run(Timer& timer)
{
    timer.Start();

    const SizeT NumObjects = 1000000;
    void** ptrs = (void**) Memory::Alloc(Memory::DefaultHeap, NumObjects * sizeof(void*));
    
    MemoryPool pool;
    pool.Setup(Memory::DefaultHeap, 13, NumObjects);
    
    Timer memPoolTimer;

    // allocate / free from memory pool
    IndexT run;
    for (run = 0; run < 5; run++)
    {
        // allocate...
        memPoolTimer.Reset();
        memPoolTimer.Start();
        IndexT i;
        for (i = 0; i < NumObjects; i++)
        {
            ptrs[i] = pool.Alloc();
        }
        memPoolTimer.Stop();
        n_printf("Run %d: Allocate %d memory pool blocks: %f\n", run, NumObjects, memPoolTimer.GetTime());

        // free...
        memPoolTimer.Reset();
        memPoolTimer.Start();
        for (i = 0; i < NumObjects; i++)
        {
            pool.Free(ptrs[i]);
        }
        memPoolTimer.Stop();
        n_printf("Run %d: Free %d memory pool blocks: %f\n", run, NumObjects, memPoolTimer.GetTime());
    }

    // conventional alloc / free
    for (run = 0; run < 5; run++)
    {
        // allocate...
        memPoolTimer.Reset();
        memPoolTimer.Start();
        IndexT i;
        for (i = 0; i < NumObjects; i++)
        {
            ptrs[i] = Memory::Alloc(Memory::DefaultHeap, 13);
        }
        memPoolTimer.Stop();
        n_printf("Run %d: Allocate %d blocks: %f\n", run, NumObjects, memPoolTimer.GetTime());

        // free...
        memPoolTimer.Reset();
        memPoolTimer.Start();
        for (i = 0; i < NumObjects; i++)
        {
            Memory::Free(Memory::DefaultHeap, ptrs[i]);
        }
        memPoolTimer.Stop();
        n_printf("Run %d: Free %d blocks: %f\n", run, NumObjects, memPoolTimer.GetTime());
    }

    Memory::Free(Memory::DefaultHeap, ptrs);
    timer.Stop();
}

} // namespace Benchmarking