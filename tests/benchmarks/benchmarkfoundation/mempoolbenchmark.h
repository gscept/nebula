#pragma once
//------------------------------------------------------------------------------
/** 
    @class Benchmarking::MemPoolBenchmark
    
    Test memory pool performance.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class MemPoolBenchmark : public Benchmark
{
    __DeclareClass(MemPoolBenchmark);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};        

} // namespace Benchmarking
//------------------------------------------------------------------------------


    