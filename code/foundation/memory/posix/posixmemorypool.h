#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixMemoryPool
 
    FIXME: IMPLEMENT ME!
 
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file    
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixMemoryPool
{
public:
    /// constructor
    PosixMemoryPool();
    /// destructor
    ~PosixMemoryPool();
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
};
    
//------------------------------------------------------------------------------
/**
*/
#if NEBULA_MEMORY_STATS
inline uint
PosixMemoryPool::GetAllocCount() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
#endif
    
//------------------------------------------------------------------------------
/**
*/
inline uint
PosixMemoryPool::ComputeAlignedBlockSize(uint blockSize)
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
PosixMemoryPool::GetNumBlocks() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
PosixMemoryPool::GetBlockSize() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
PosixMemoryPool::GetAlignedBlockSize() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline uint
PosixMemoryPool::GetPoolSize() const
{
    n_error("NOT IMPLEMENTED!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
inline bool
PosixMemoryPool::IsPoolBlock(void* ptr) const
{
    n_error("NOT IMPLEMENTED!\n");
    return false;
}
    
} // namespace Posix
//------------------------------------------------------------------------------

