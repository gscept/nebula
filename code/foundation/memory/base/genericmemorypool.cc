//------------------------------------------------------------------------------
//  genericmemorypool.cc
//  (C) Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "memory/base/genericmemorypool.h"
#include "util/string.h"

namespace Base
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
GenericMemoryPool::GenericMemoryPool() :
    heapType(Memory::InvalidHeapType),
    blockSize(0),
    alignedBlockSize(0),
    poolSize(0),
    numBlocks(0),
    poolStart(0),
    poolEnd(0)
{
    #if NEBULA_MEMORY_STATS
    this->allocCount = 0;
    #endif
}

//------------------------------------------------------------------------------
/**
*/
GenericMemoryPool::~GenericMemoryPool()
{
    #if NEBULA_MEMORY_STATS
    if (this->allocCount != 0)
    {
        String str;
        str.Format("GenericMemoryPool: %d memory leaks in pool '0x%08x'!\n", this->allocCount, (uintptr_t)this);
        Core::SysFunc::DebugOut(str.AsCharPtr());
    }
    #endif

    // discard memory pool
    Memory::Free(this->heapType, this->poolStart);
    this->poolStart = 0;
    this->poolEnd = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
GenericMemoryPool::Setup(Memory::HeapType heapType_, uint blockSize_, uint numBlocks_)
{
    n_assert(0 == this->poolStart);
    n_assert(0 == this->poolEnd);
    this->blockSize = blockSize_;
    this->numBlocks = numBlocks_;
    this->heapType = heapType_;

    #if NEBULA_MEMORY_STATS
    this->allocCount = 0;
    #endif

    // compute block size with sizeof(*) header
    this->alignedBlockSize = ComputeAlignedBlockSize(this->blockSize);
    this->poolSize = (this->numBlocks * this->alignedBlockSize) + BlockAlign;

    // setup pool memory block, each entry in the block has a header
    // with a pointer to the next block, the actual start of the memory block
    // is 16-byte-aligned
    this->poolStart = (ubyte*) Memory::Alloc(this->heapType, this->poolSize);
    this->poolEnd = this->poolStart + this->poolSize;

    uint i;
    ubyte* blockEntry = this->poolStart;
    for (i = 0; i < this->numBlocks; i++)
    {
        this->freeList.Enqueue(blockEntry);
        blockEntry += this->alignedBlockSize; 
    }
}

//------------------------------------------------------------------------------
/**
*/
void*
GenericMemoryPool::Alloc()
{
    #if NEBULA_MEMORY_STATS
    Threading::Interlocked::Increment(&this->allocCount);
    #endif
    // get the next free block from the free list and fixup the free list
    ubyte* entry = this->freeList.DequeueSafe(nullptr);
    if (0 == entry)
    {
        return 0;
    }
    void* ptr = (void*) (entry);

    // fill with debug pattern
    #if NEBULA_DEBUG
    Memory::Fill(ptr, this->blockSize, NewBlockPattern);
    #endif

    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
void
GenericMemoryPool::Free(void* ptr)
{
    #if NEBULA_MEMORY_STATS
    n_assert(this->allocCount > 0);
    Threading::Interlocked::Decrement(&this->allocCount);
    #endif
    this->freeList.Enqueue((ubyte*)ptr);

    // fill free'd block with debug pattern
    #if NEBULA_DEBUG
    Memory::Fill(ptr, this->blockSize, FreeBlockPattern);
    #endif
}

} // namespace Win32
