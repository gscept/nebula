#pragma once
//------------------------------------------------------------------------------
/**
	@class Test::ArrayAllocatorTest

	Tests Nebula's array allocator class.

	(C) 2018 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ArrayAllocatorTest : public TestCase
{
	__DeclareClass(ArrayAllocatorTest);
public:
	/// run the test
	virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
