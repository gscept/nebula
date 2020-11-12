//------------------------------------------------------------------------------
//  runbenchmarks.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "benchmarkbase/benchmarkrunner.h"

#include "querybenchmark.h"

using namespace Core;
using namespace Benchmarking;

void __cdecl
main()
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Benchmark Runner"));
    coreServer->Open();

    // setup and run benchmarks
    Ptr<BenchmarkRunner> runner = BenchmarkRunner::Create();    
    runner->AttachBenchmark(QueryBenchmark::Create());
    runner->Run();
    
    // shutdown Nebula runtime
    runner = nullptr;
    coreServer->Close();
    coreServer = nullptr;
    SysFunc::Exit(0);
}
