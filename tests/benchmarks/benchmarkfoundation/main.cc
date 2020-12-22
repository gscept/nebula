//------------------------------------------------------------------------------
//  runbenchmarks.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "benchmarkbase/benchmarkrunner.h"

#include "createobjects.h"
#include "createobjectsbyfourcc.h"
#include "createobjectsbyclassname.h"
#include "float4math.h"
#include "matrix44inverse.h"
#include "matrix44multiply.h"
#include "mempoolbenchmark.h"
#include "containerbenchmark.h"
#include "delegates.h"

using namespace Core;
using namespace Benchmarking;

int __cdecl
main(int argc, char** argv)
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Benchmark Runner"));
    coreServer->Open();

    // setup and run benchmarks
    Ptr<BenchmarkRunner> runner = BenchmarkRunner::Create();    
    runner->AttachBenchmark(Matrix44Multiply::Create());
    runner->AttachBenchmark(Matrix44Inverse::Create());
    runner->AttachBenchmark(Float4Math::Create());
    runner->AttachBenchmark(MemPoolBenchmark::Create());
    runner->AttachBenchmark(CreateObjects::Create());
    runner->AttachBenchmark(CreateObjectsByFourCC::Create());
    runner->AttachBenchmark(CreateObjectsByClassName::Create());
    runner->AttachBenchmark(ContainerBench::Create());
    runner->AttachBenchmark(DelegateBench::Create());
    runner->Run();
    
    // shutdown Nebula runtime
    runner = nullptr;
    coreServer->Close();
    coreServer = nullptr;
    SysFunc::Exit(0);
    return 0;
}
