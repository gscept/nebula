#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::PoolArrayAllocator
  
    Allocates small memory blocks from an array of fixed-size memory pools.
    Bigger allocation go directly through to a heap. Note that when
    freeing a memory block, there are 2 options: one with providing
    the size of the memory block (which is probably a bit faster) and
    one conventional without providing the size.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "memory/memorypool.h"

//------------------------------------------------------------------------------
namespace Memory
{
class PoolArrayAllocator
{
public:
    /// number of pools
    static const SizeT NumPools = 8;

    /// constructor
    PoolArrayAllocator();
    /// destructor
    ~PoolArrayAllocator();
    
    /// setup the pool allocator, name must be a static string!
    void Setup(const char* name, Memory::HeapType heapType, uint poolSizes[NumPools]);
    /// allocate a block of memory from the pool
    void* Alloc(SizeT size);
    /// free a block of memory from the pool array with original block size
    void Free(void* ptr, SizeT size);
    /// free a block of memory from the pool array
    void Free(void* ptr);
    /// access to memory pool at pool index (for debugging)
    const MemoryPool& GetMemoryPool(IndexT index) const;

    #if NEBULA_MEMORY_STATS
    /// dump number of memory allocations (if > 0)
    void Dump();
    #endif

private:
    Memory::HeapType heapType;
    const char* name;
    MemoryPool memoryPools[NumPools];
};

} // namespace Memory
//------------------------------------------------------------------------------
