#pragma once
//------------------------------------------------------------------------------
/** 
    @class Benchmarking::QueryBenchmark
    
    Test query performance
    
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class QueryBenchmark : public Benchmark
{
    __DeclareClass(QueryBenchmark);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};        

} // namespace Benchmarking
//------------------------------------------------------------------------------


    