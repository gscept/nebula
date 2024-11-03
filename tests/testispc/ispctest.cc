//------------------------------------------------------------------------------
//  ispctest.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ispctest.h"
#include "io/ioserver.h"
#include "testispc/test.h"

namespace Test
{
__ImplementClass(Test::ISPCTest, 'istp', Test::TestCase);

using namespace System;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
ISPCTest::Run()
{
    float vin[16], vout[16];
    for (int i = 0; i < 16; ++i)
    {
        vin[i] = i;
    }

    ispc::ispc_test(vin, vout, 16);

    bool match = true;

    for (int i = 0; i < 16; ++i)
    {
        match &= vin[i] * vin[i] == vout[i];
    }
    VERIFY(match);
    //VERIFY(test2Item.name == testItem.name);
    //VERIFY(test2Item.pos == testItem.pos);
}

} // namespace Test