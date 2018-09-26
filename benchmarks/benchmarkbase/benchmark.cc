//------------------------------------------------------------------------------
//  benchmark.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "benchmark.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::Benchmark, 'BNCH', Core::RefCounted);

using namespace Timing;

//------------------------------------------------------------------------------
/**
    Overwrite this method in a subclass and use the provided timer
    to measure the time spent in the benchmark. Usually you just need
    to call the Start() and Stop() method of the timer around the piece
    of code you want to benchmark.
*/
void
Benchmark::Run(Timer& timer)
{
    timer.Start();
    timer.Stop();
}

} // namespace Benchmark
