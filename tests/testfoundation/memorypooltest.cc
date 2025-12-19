//------------------------------------------------------------------------------
//  memorypooltest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memorypooltest.h"
#include "memory/memorypool.h"

namespace Test
{
__ImplementClass(Test::MemoryPoolTest, 'MEPT', Test::TestCase);

using namespace Memory;

//------------------------------------------------------------------------------
/**
*/
void
MemoryPoolTest::Run()
{
// FIXME FIXME FIXME

    const SizeT blockSize = 45;
    const SizeT numBlocks = 4;
    MemoryPool memPool;
    memPool.Setup(Memory::DefaultHeap, blockSize, numBlocks);

    VERIFY(memPool.GetBlockSize() == blockSize);
	VERIFY(memPool.GetNumBlocks() == numBlocks);

    // do some allocations
    void* ptr[numBlocks] = { 0 };
    IndexT i;
    for (i = 0; i < numBlocks; i++)
    {
        ptr[i] = memPool.Alloc();
		VERIFY(0 != ptr[i]);
    }
	// next allocation should fail
	void* failPtr = memPool.Alloc();
	VERIFY(0 == failPtr);

    // ..and free them again
    for (i = 0; i < numBlocks; i++)
    {
        memPool.Free(ptr[i]);
    }

}

} // namespace Test
