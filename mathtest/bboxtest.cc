//------------------------------------------------------------------------------
//  bboxtest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "bboxtest.h"
#include "math/float4.h"
#include "math/matrix44.h"
#include "math/bbox.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;

namespace Test
{
__ImplementClass(Test::BBoxTest, 'BBTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
BBoxTest::Run()
{
	STACK_CHECKPOINT("Test::BBoxTest::Run()");
	Math::matrix44 cam = Math::matrix44::perspfovlh(60.0f, 16.0f / 9.0f, 0.0001f, 1000.0f);

	static const int NumBoxes = 100;
	for (IndexT i = -NumBoxes; i < NumBoxes; i+=10)
	{
		for (IndexT j = -NumBoxes; j < NumBoxes; j+=10)
		{
			Math::bbox box;
			box.set(Math::matrix44::multiply(Math::matrix44::scaling(10, 10, 10), Math::matrix44::translation(j, 0, i)));

			ClipStatus::Type s = box.clipstatus(cam);
			VERIFY(s == box.clipstatus_simd(cam));
		}
	}
}

}