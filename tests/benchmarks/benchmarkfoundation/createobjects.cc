//------------------------------------------------------------------------------
//  createobjects.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "createobjects.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::CreateObjects, 'BCOB', Benchmarking::Benchmark);

using namespace Core;
using namespace Timing;

//------------------------------------------------------------------------------
/**
*/
void
CreateObjects::Run(Timer& timer)
{
    // create a million RefCounted objects
    const int numObjects = 1000000;
    Ptr<RefCounted> ptr;
    timer.Start();
    int i;
    for (i = 0; i < numObjects; i++)
    {
        ptr = RefCounted::Create();
    }
    timer.Stop();
}

} // namespace Benchmarking
