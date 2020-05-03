#pragma once
//------------------------------------------------------------------------------
/**
    @class Benchmarking::CreateObjectsByClassName
  
    Create many objects by their class name.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class CreateObjectsByClassName : public Benchmark
{
    __DeclareClass(CreateObjectsByClassName);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------