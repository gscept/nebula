#ifdef __USE_MATH_DIRECTX
//------------------------------------------------------------------------------
//  transformtest.cc
//  2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "transformtest.h"
#include "math/transform.h"
#include "math/quaternion.h"
#include "math/point.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;

namespace Test
{
__ImplementClass(Test::TransformTest, 'TFTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
TransformTest::Run()
{
    STACK_CHECKPOINT("Test::TransformTest::Run() begin");

    transform t1;
    quaternion q1(1.0f, 2.0f, 3.0f, 4.0f);
    point p1(5.0f, 6.0f, 7.0f);

    t1.set(q1, p1);
    this->Verify(t1.get_position() == p1);
    this->Verify(t1.get_orientation() == q1);
    matrix44 foo;
    t1.to_matrix44(foo);
    this->Verify(t1.get_orientation() == q1);
    matrix44 mm = matrix44::affinetransformation(1.0, float4(0), q1, p1);
    this->Verify(mm == foo);
}

}
#endif
