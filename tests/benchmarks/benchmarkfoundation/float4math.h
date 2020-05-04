#pragma once
//------------------------------------------------------------------------------
/**
    @class Benchmarking::Float4Math
    
    Test float4 misc functions performance.
    
    (C) 2007 Radon Labs 
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class Float4Math : public Benchmark
{
    __DeclareClass(Float4Math);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------