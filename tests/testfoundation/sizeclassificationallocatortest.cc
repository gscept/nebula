//------------------------------------------------------------------------------
//  pinnedarraytest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/array.h"
#include "sizeclassificationallocatortest.h"
#include "memory/rangeallocator.h"

namespace Test
{
__ImplementClass(Test::SizeClassificationAllocatorTest, 'SCAT', Test::TestCase);

using namespace Util;


//------------------------------------------------------------------------------
/**
*/
void
SizeClassificationAllocatorTest::Run()
{
    using namespace Memory;
    RangeAllocator allocator(256, 4);
    RangeAllocation alloc1 = allocator.Alloc(64);
    RangeAllocation alloc2 = allocator.Alloc(64);
    RangeAllocation alloc3 = allocator.Alloc(64);
    RangeAllocation alloc4 = allocator.Alloc(64);
    VERIFY(alloc1.offset == 0);
    VERIFY(alloc2.offset == 64);
    VERIFY(alloc3.offset == 128);
    VERIFY(alloc4.offset == 192);

    // Freeing up two consequtive blocks of 64 should allow for one free of 128
    allocator.Dealloc(alloc2);
    allocator.Dealloc(alloc3);
    RangeAllocation alloc5 = allocator.Alloc(128);
    VERIFY(alloc2.offset == 64);

    // If we allocate now, it should be oom
    RangeAllocation alloc6 = allocator.Alloc(64);
    VERIFY(alloc6.offset == alloc6.OOM);

    // Empty the allocator
    allocator.Clear();

    // Should reset allocator and new offset should be 0
    alloc1 = allocator.Alloc(64);
    VERIFY(alloc1.offset == 0);

    // Allocating with a huge alignment should fail
    alloc2 = allocator.Alloc(64, 256);
    VERIFY(alloc2.offset == alloc2.OOM);

    // Allocating with 128 alignment should work
    alloc3 = allocator.Alloc(64, 128);
    VERIFY(alloc3.offset == 128);

    // We should be able to fit two more allocs, if they are not aligned (fill alignment gap)
    alloc4 = allocator.Alloc(64);
    VERIFY(alloc4.offset == 64);
    alloc5 = allocator.Alloc(64);
    VERIFY(alloc5.offset == 192);

    // However, allocating another with 128 padding should fail
    allocator.Dealloc(alloc5);
    alloc5 = allocator.Alloc(64, 128);
    VERIFY(alloc5.offset = alloc5.OOM);
}

}; // namespace Test