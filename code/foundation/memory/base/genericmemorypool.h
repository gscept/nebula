#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::GenericMemoryPool
    
    A simple thread-safe memory pool. Memory pool items are 16-byte aligned.
    
    FIXME:
    - debug: overwrite memory blocks with pattern 
    - debug: check for double-free
    - debug: check for mem-leaks
    - debug: list memory pools in Debug HTML page!

    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "threading/safequeue.h"

//------------------------------------------------------------------------------
namespace Base
{
class GenericMemoryPool
{
public:
    /// constructor
    GenericMemoryPool();
    /// destructor
    ~GenericMemoryPool();
    /// compute the actual block size including alignment and management data
    static uint ComputeAlignedBlockSize(uint blockSize);
    /// setup the memory pool
    void Setup(Memory::HeapType heapType, uint blockSize, uint numBlocks);
    /// allocate a block from the pool (NOTE: returns 0 if pool exhausted!)
    void* Alloc();
    /// deallocate a block from the pool
    void Free(void* ptr);
    /// return true if block is owned by this pool
    bool IsPoolBlock(void* ptr) const;
    /// get number of allocated blocks in pool
    uint GetNumBlocks() const;
    /// get block size
    uint GetBlockSize() const;
    /// get aligned block size
    uint GetAlignedBlockSize() const;
    /// get pool size
    uint GetPoolSize() const;
    /// get current allocation count
    #if NEBULA_MEMORY_STATS
    uint GetAllocCount() const;
    #endif

private:
    static const uint FreeBlockPattern = 0xFE;
    static const uint NewBlockPattern = 0xFD;
    static const int  BlockAlign = 16;

    Memory::HeapType heapType;
    uint blockSize;
    uint alignedBlockSize;
    uint poolSize;
    uint numBlocks;
    #if NEBULA_MEMORY_STATS
    volatile long allocCount;
    #endif
    
    Threading::SafeQueue<ubyte*> freeList;
    ubyte* poolStart;
    ubyte* poolEnd;
};

//------------------------------------------------------------------------------
/**
*/
#if NEBULA_MEMORY_STATS
inline uint
GenericMemoryPool::GetAllocCount() const
{
    return this->allocCount;
}
#endif

//------------------------------------------------------------------------------
/**
*/
inline uint
GenericMemoryPool::ComputeAlignedBlockSize(uint blockSize)
{
    uint blockSizeWithHeader = blockSize;
    uint padding = (BlockAlign - (blockSizeWithHeader % BlockAlign)) % BlockAlign;
    return (blockSizeWithHeader + padding);
}

//------------------------------------------------------------------------------
/**
*/
inline uint
GenericMemoryPool::GetNumBlocks() const
{
    return this->numBlocks;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
GenericMemoryPool::GetBlockSize() const
{
    return this->blockSize;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
GenericMemoryPool::GetAlignedBlockSize() const
{
    return this->alignedBlockSize;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
GenericMemoryPool::GetPoolSize() const
{
    return this->poolSize;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
GenericMemoryPool::IsPoolBlock(void* ptr) const
{
    return (ptr >= this->poolStart) && (ptr < this->poolEnd);
}

} // namespace Base
//------------------------------------------------------------------------------
    