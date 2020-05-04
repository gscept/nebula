//------------------------------------------------------------------------------
//  benchmarkrunner.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "benchmarkrunner.h"
#include "timing/timer.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::BenchmarkRunner, 'BNCR', Core::RefCounted);

using namespace Util;
using namespace Timing;

//------------------------------------------------------------------------------
/**
*/
void
BenchmarkRunner::AttachBenchmark(Benchmark* b)
{
    n_assert(0 != b);
    this->benchmarks.Append(b);
}

//------------------------------------------------------------------------------
/**
*/
void
BenchmarkRunner::Run()
{
    n_printf("---------------------------------------------------------------\n");
    uint overallTicks = 0;
    Time overallSeconds = 0.0;
    IndexT i;
    SizeT num = this->benchmarks.Size();
    for (i = 0; i < num; i++)
    {
        Benchmark* b = this->benchmarks[i];
        Timer timer;
        b->Run(timer);
        uint ticks = timer.GetTicks();
        Time seconds = timer.GetTime();

        Array<String> tokens = b->GetClassName().Tokenize(":");
        const String& className = tokens[tokens.Size() - 1];

        n_printf("> %s: %d ticks, %f seconds\n", className.AsCharPtr(), ticks, seconds);
        overallTicks += ticks;
        overallSeconds += seconds;
    }
    n_printf("---------------------------------------------------------------\n");
    n_printf("* OVERALL: %d ticks, %f seconds\n", overallTicks, overallSeconds);
}

} // namespace Benchmark
