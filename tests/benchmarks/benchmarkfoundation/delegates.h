#pragma once
//------------------------------------------------------------------------------
/**
    @class Benchmarking::DelegateBench

    Test delegates performance vs. direct call, functions pointer and std::function.

    (C) 2019 Individual contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class DelegateBench : public Benchmark
{
    __DeclareClass(DelegateBench);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------