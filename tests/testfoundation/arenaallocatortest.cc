//------------------------------------------------------------------------------
//  arenaallocatortest.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "arenaallocatortest.h"
#include "memory/arenaallocator.h"

namespace Test
{
__ImplementClass(Test::ArenaAllocatorTest, 'ARNA', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
ArenaAllocatorTest::Run()
{
    // small allocations are packed into the current chunk with 16 byte padding
    {
        Memory::ArenaAllocator<256> arena;
        int* a = arena.Alloc<int>();
        int* b = arena.Alloc<int>();
        VERIFY((byte*)b == (byte*)a + 16);
    }

    // an array allocation larger than a chunk used to be placed in a regular chunk,
    // overflowing it, it has to get its own block and leave the current chunk alone
    {
        Memory::ArenaAllocator<256> arena;
        int* first = arena.Alloc<int>();
        *first = 0x1234;
        int* big = arena.Alloc<int>(100);
        int* second = arena.Alloc<int>();
        bool separateBlock = (byte*)second == (byte*)first + 16;
        VERIFY(separateBlock);

        // only touch the range if it is backed by its own block, otherwise this
        // test would corrupt the heap when the bug comes back
        if (separateBlock)
        {
            *second = 0x5678;
            for (int i = 0; i < 100; i++)
                big[i] = i;
            bool contentsOk = true;
            for (int i = 0; i < 100; i++)
                contentsOk &= big[i] == i;
            VERIFY(contentsOk);
            VERIFY(*first == 0x1234);
            VERIFY(*second == 0x5678);
        }
    }

    // an array allocation which fits a chunk stays in the chunk
    {
        Memory::ArenaAllocator<256> arena;
        int* first = arena.Alloc<int>();
        int* fits = arena.Alloc<int>(4);
        VERIFY((byte*)fits == (byte*)first + 16);
    }

    // raw allocations larger than a chunk get their own block as well
    {
        Memory::ArenaAllocator<256> arena;
        byte* first = (byte*)arena.Alloc(16);
        byte* big = (byte*)arena.Alloc(1000);
        byte* second = (byte*)arena.Alloc(16);
        VERIFY(second == first + 16);
        VERIFY(big != nullptr && (big + 1000 <= first || big >= first + 256));
    }
}

}; // namespace Test
