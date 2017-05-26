//------------------------------------------------------------------------------
//  memorypooltest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memorypooltest.h"
//#include "memory/memorypool.h"

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
/*
    const SizeT blockSize = 45;
    const SizeT numBlocks = 4;
    MemoryPool memPool("TestPool", blockSize, numBlocks);

    this->Verify(Util::String("TestPool") == memPool.GetName());
    this->Verify(memPool.GetBlockSize() == blockSize);

    // do some allocations
    void* ptr[numBlocks] = { 0 };
    IndexT i;
    for (i = 0; i < numBlocks; i++)
    {
        ptr[i] = memPool.Alloc();
    }

    // ..and free them again
    for (i = 0; i < numBlocks; i++)
    {
        memPool.Free(ptr[i]);
    }
*/
}

} // namespace Test
