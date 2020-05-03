#pragma once
//------------------------------------------------------------------------------
/**
    @class Benchmarking::CreateObjectsByFourCC
  
    Create many objects by their class FourCC code.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file

*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class CreateObjectsByFourCC : public Benchmark
{
    __DeclareClass(CreateObjectsByFourCC);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------