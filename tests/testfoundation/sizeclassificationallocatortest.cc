//------------------------------------------------------------------------------
//  pinnedarraytest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/array.h"
#include "sizeclassificationallocatortest.h"
#include <functional>
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

    // Allocating with 128 alignment should work, and should be padded enough to hold the value aligned
    alloc3 = allocator.Alloc(64, 128);
    VERIFY(alloc3.offset == 128);

    // The memory starts at byte 128 and needs to be 64 byte long
    VERIFY((alloc3.offset + 64 - 1) < alloc3.size);

    // The previous insertion actually inserts a 192 byte block due to alignment, so this should fail
    alloc4 = allocator.Alloc(64);
    VERIFY(alloc4.offset == alloc4.OOM);

    uint randomAlignments[] = { 64, 128, 256 };

    RangeAllocator randomAllocator(20000, 2048);
    Util::Array<uint> pending = { 10000, 5000, 10000, 2500, 7500, 5000 };
    while (!pending.IsEmpty())
    {
        Util::Array<RangeAllocation> allocations;
        Util::Array<uint> indices;
        uint counter = 0;
        for (int i = pending.Size() - 1; i >= 0; i--)
        {
            auto a = randomAllocator.Alloc(pending[i], randomAlignments[i % 3]);
            if (a.offset != a.OOM)
            {
                allocations.Append(a);
                pending.PopBack();
                indices.Append(counter++);
            }
        }

        for (int i = indices.Size() - 1; i >= 0; i--)
        {
            randomAllocator.Dealloc(allocations[indices[i]]);
        }
    }

    VERIFY(randomAllocator.freeStorage == randomAllocator.size);

}

}; // namespace Test