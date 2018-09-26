#ifndef BENCHMARKING_BENCHMARKRUNNER_H
#define BENCHMARKING_BENCHMARKRUNNER_H
//------------------------------------------------------------------------------
/**
    @class Benchmarking::BenchmarkRunner

    The benchmark runner class which runs all benchmarks.
    
    (C) 2006 Radon Labs GmbH
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "util/array.h"
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class BenchmarkRunner : public Core::RefCounted
{
    __DeclareClass(BenchmarkRunner);
public:
    /// attach a benchmark
    void AttachBenchmark(Benchmark* b);
    /// run the benchmarks
    void Run();

private:
    Util::Array<Ptr<Benchmark>> benchmarks;
};

} // namespace Benchmark
//------------------------------------------------------------------------------
#endif


