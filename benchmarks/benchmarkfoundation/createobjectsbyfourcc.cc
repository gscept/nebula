//------------------------------------------------------------------------------
//  createobjectsbyfourcc.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "createobjectsbyfourcc.h"
#include "core/factory.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::CreateObjectsByFourCC, 'BCOF', Benchmarking::Benchmark);

using namespace Core;
using namespace Timing;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
CreateObjectsByFourCC::Run(Timer& timer)
{
    // create a million RefCounted objects
    const int numObjects = 1000000;
    Factory* factory = Factory::Instance();
    FourCC classFourCC('REFC');
    Ptr<RefCounted> ptr;
    int i;
    timer.Start();
    for (i = 0; i < numObjects; i++)
    {
        ptr = static_cast<RefCounted*>(factory->Create(classFourCC));
    }
    timer.Stop();
}

} // namespace Benchmarking
