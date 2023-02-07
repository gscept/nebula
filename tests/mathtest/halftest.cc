//------------------------------------------------------------------------------
//  halftest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "halftest.h"
#include "math/half.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;

namespace Test
{
__ImplementClass(Test::HalfTest, 'HLTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
HalfTest::Run()
{
    STACK_CHECKPOINT("Test::HalfTest::Run()");

    Math::half h0(0.5f), h1(1.0f);
    VERIFY(h0 + h1 == Math::half(1.5f));
    VERIFY(h0 + h1 == 1.5f);
    VERIFY(h1 > h0);
    VERIFY(h0 < h1);
    VERIFY(h0 != h1);

    h0 += 0.5f;
    VERIFY(h0 == 1.0f);

    h1 = 60000.0f;
    VERIFY(h1 != std::numeric_limits<Math::half>::infinity());
    h1 += 60000.0f;
    VERIFY(h1 == std::numeric_limits<Math::half>::infinity());
}

} // namespace Test