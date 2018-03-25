#pragma once
//------------------------------------------------------------------------------
/**
	Tests block allocator
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "testbase/testcase.h"
namespace Test
{
class BlockAllocatorTest : public TestCase
{
	__DeclareClass(BlockAllocatorTest);
public:
	/// run test
	virtual void Run();
};
} // namespace Test