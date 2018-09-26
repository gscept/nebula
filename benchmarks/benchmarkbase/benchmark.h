#ifndef BENCHMARKING_BENCHMARK_H
#define BENCHMARKING_BENCHMARK_H
//------------------------------------------------------------------------------
/**
    @class Benchmarking::Benchmark
    
    Base class for Nebula3 benchmarks.
    
    (C) 2006 Radon Labs GmbH
*/
#include "core/refcounted.h"
#include "timing/timer.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class Benchmark : public Core::RefCounted
{
    __DeclareClass(Benchmark);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------
#endif
