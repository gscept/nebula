#pragma once
//------------------------------------------------------------------------------
/**
    @class Benchmarking::Matrix44Multiply
    
    Test matrix44::multiply() performance.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class Matrix44Multiply : public Benchmark
{
    __DeclareClass(Matrix44Multiply);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------
