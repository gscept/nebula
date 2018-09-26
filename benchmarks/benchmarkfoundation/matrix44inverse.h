#pragma once
//------------------------------------------------------------------------------
/**
    @class Benchmarking::Matrix44Inverse
    
    Test matrix44::inverse() performance.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class Matrix44Inverse : public Benchmark
{
    __DeclareClass(Matrix44Inverse);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------
