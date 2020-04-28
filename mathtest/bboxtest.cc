//------------------------------------------------------------------------------
//  bboxtest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "bboxtest.h"
#include "math/vec4.h"
#include "math/mat4.h"
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
	Math::mat4 cam = Math::perspfovlh(60.0f, 16.0f / 9.0f, 0.0001f, 1000.0f);

	static const int NumBoxes = 100;
	for (IndexT i = -NumBoxes; i < NumBoxes; i+=10)
	{
		for (IndexT j = -NumBoxes; j < NumBoxes; j+=10)
		{
			Math::bbox box;
			box.set(Math::scaling(10, 10, 10) * Math::translation(j, 0, i));
		}
	}
}

}