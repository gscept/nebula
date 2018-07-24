//------------------------------------------------------------------------------
//  arrayallocatortest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "arrayallocatortest.h"
#include "util/arrayallocator.h"

namespace Test
{
__ImplementClass(Test::ArrayAllocatorTest, 'AALT', Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
ArrayAllocatorTest::Run()
{
	ArrayAllocator<int, float, unsigned char, bool> allocator;

	uint32_t id = allocator.Alloc();

	this->Verify(id == 0);
	this->Verify(allocator.Size() == 1);

	allocator.EraseIndex(id);

	this->Verify(allocator.Size() == 0);

	for (SizeT i = 0; i < 20000; i++)
	{
		allocator.Alloc();
		allocator.Get<0>(i) = i;
		allocator.Get<1>(i) = (float)i * 2.0f;
		allocator.Get<2>(i) = i % 10;
		allocator.Get<3>(i) = bool(i & 1);
	}

	this->Verify(allocator.Size() == 20000);

	for (SizeT i = 0; i < 10000; i++)
	{
		allocator.EraseIndexSwap(i);
	}

	this->Verify(allocator.Size() == 10000);

	this->Verify(allocator.Get<0>(0) == 19999);
	this->Verify(allocator.Get<1>(1) == 19998.0f * 2.0f);
	this->Verify(allocator.Get<2>(2) == 19997 % 10);
	this->Verify(allocator.Get<3>(3) == bool(19996 & 1));

	this->Verify(allocator.GetArray<0>().Size() == 10000);
	this->Verify(allocator.GetArray<1>().Size() == 10000);
	this->Verify(allocator.GetArray<2>().Size() == 10000);
	this->Verify(allocator.GetArray<3>().Size() == 10000);

	allocator.Clear();

	this->Verify(allocator.GetArray<0>().Size() == 0);
	this->Verify(allocator.GetArray<1>().Size() == 0);
	this->Verify(allocator.GetArray<2>().Size() == 0);
	this->Verify(allocator.GetArray<3>().Size() == 0);

	this->Verify(allocator.Size() == 0);	
}

} // namespace Test
