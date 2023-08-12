//------------------------------------------------------------------------------
//  win32memorypool.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "memory/win32/win32memorypool.h"
#include "util/string.h"

namespace Win32
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
Win32MemoryPool::Win32MemoryPool() :
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
    Memory::Clear(&this->listHead, sizeof(this->listHead));
}

//------------------------------------------------------------------------------
/**
*/
Win32MemoryPool::~Win32MemoryPool()
{
    #if NEBULA_MEMORY_STATS
    if (this->allocCount != 0)
    {
        String str;
        str.Format("Win32MemoryPool: %d memory leaks in pool '0x%08x'!\n", this->allocCount, (uintptr_t)this);
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
    NOTE: name must be a static string!
*/
void
Win32MemoryPool::Setup(Memory::HeapType heapType_, uint blockSize_, uint numBlocks_)
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
    
    // setup forward-linked free-block-list
    Memory::Clear(&this->listHead, sizeof(listHead));
    PSLIST_ENTRY listEntry = (PSLIST_ENTRY) (this->poolStart + (BlockAlign - sizeof(SLIST_ENTRY)));
    uint i;
    for (i = 0; i < this->numBlocks; i++)
    {
        InterlockedPushEntrySList(&this->listHead, listEntry);
        listEntry += this->alignedBlockSize / sizeof(SLIST_ENTRY);
    }
}

//------------------------------------------------------------------------------
/**
*/
void*
Win32MemoryPool::Alloc()
{
    #if NEBULA_MEMORY_STATS
    _InterlockedIncrement(&this->allocCount);
    #endif
    // get the next free block from the free list and fixup the free list
    PSLIST_ENTRY entry = InterlockedPopEntrySList(&this->listHead);
    if (0 == entry)
    {
        return 0;
    }
    void* ptr = (void*) (((ubyte*)entry) + sizeof(SLIST_ENTRY));

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
Win32MemoryPool::Free(void* ptr)
{
    #if NEBULA_MEMORY_STATS
    n_assert(this->allocCount > 0);
    _InterlockedDecrement(&this->allocCount);
    #endif
    // get pointer to header and fixup free list
    PSLIST_ENTRY entry = (PSLIST_ENTRY) (((ubyte*)ptr) - sizeof(SLIST_ENTRY));
    InterlockedPushEntrySList(&this->listHead, entry);

    // fill free'd block with debug pattern
    #if NEBULA_DEBUG
    Memory::Fill(ptr, this->blockSize, FreeBlockPattern);
    #endif
}

} // namespace Win32
