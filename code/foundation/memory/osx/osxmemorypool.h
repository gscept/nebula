#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::OSXMemoryPool
 
    FIXME: IMPLEMENT ME!
 
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace OSX
{
class OSXMemoryPool
{
public:
    /// constructor
    OSXMemoryPool();
    /// destructor
    ~OSXMemoryPool();
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
#if NEBULA3_MEMORY_STATS
    uint GetAllocCount() const;
#endif
};
    
//------------------------------------------------------------------------------
/**
*/
#if NEBULA3_MEMORY_STATS
inline uint
OSXMemoryPool::GetAllocCount() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
#endif
    
//------------------------------------------------------------------------------
/**
*/
inline uint
OSXMemoryPool::ComputeAlignedBlockSize(uint blockSize)
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
OSXMemoryPool::GetNumBlocks() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
OSXMemoryPool::GetBlockSize() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
OSXMemoryPool::GetAlignedBlockSize() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
OSXMemoryPool::GetPoolSize() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline bool
OSXMemoryPool::IsPoolBlock(void* ptr) const
{
    n_error("NOT IMPLEMENTED!\n");
    return false;
}
    
} // namespace OSX
//------------------------------------------------------------------------------

