//------------------------------------------------------------------------------
//  poolarrayallocator.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "memory/poolarrayallocator.h"
#include "util/string.h"

namespace Memory
{

//------------------------------------------------------------------------------
/**
*/
PoolArrayAllocator::PoolArrayAllocator() :
    heapType(InvalidHeapType),
    name(0)
{
    Memory::Clear(this->memoryPools, sizeof(this->memoryPools));
}

//------------------------------------------------------------------------------
/**
*/
PoolArrayAllocator::~PoolArrayAllocator()
{
    // check for mem leaks
    #if NEBULA3_MEMORY_STATS
    this->Dump();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
void
PoolArrayAllocator::Setup(const char* name_, Memory::HeapType heapType_, uint poolSizes[NumPools])
{
    n_assert(0 != name_);
    n_assert(0 == this->name);

    // NOTE: name_ must be a static string!
    this->name = name_;
    this->heapType = heapType_;

    // setup memory pools, we take several assumptions about the memory pool:
    // - there's 4 bytes of overhead 
    // - allocations are 16-byte aligned
    // thus:
    // we start at 28 bytes for the smallest block size, this gives us
    // 32 byte blocks in the memory pool, and for each following
    // block size we add 32, thus we end up with the following block
    // sizes for 8 pools:
    // 28 -> 60 -> 92 -> 124 -> 156 -> 188 -> 220 -> 252
    SizeT curBlockSize = 28;
    IndexT i;
    for (i = 0; i < NumPools; i++)
    {
        n_assert(poolSizes[i] > 0);
        SizeT alignedBlockSize = MemoryPool::ComputeAlignedBlockSize(curBlockSize);
        n_assert((curBlockSize + 4) == alignedBlockSize);
        n_assert(alignedBlockSize == (32 * (i + 1)));
        SizeT curNumBlocks = poolSizes[i] / alignedBlockSize;
        this->memoryPools[i].Setup(this->heapType, curBlockSize, curNumBlocks);
        curBlockSize += 32;
    }
}

//------------------------------------------------------------------------------
/**
*/
void*
PoolArrayAllocator::Alloc(SizeT size)
{
    IndexT poolIndex = (size + 3) >> 5;
    if (poolIndex < NumPools)
    {
        #if NEBULA_DEBUG
        n_assert(uint(size) <= memoryPools[poolIndex].GetBlockSize());
        #endif
        void* ptr = this->memoryPools[poolIndex].Alloc();
        if (0 == ptr)
        {
            n_error("PoolArrayAllocator '%s': pool with block size '%d' full!\n",
                this->name, this->memoryPools[poolIndex].GetBlockSize());
        }
        return ptr;
    }
    else
    {
        // size too big, need to allocate directly from the heap
        #if NEBULA_DEBUG
        // n_printf("WARNING: Allocation of size '%d' in PoolArrayAllocator '%s' going directly to heap!\n", size, this->name);        
        #endif
        return Memory::Alloc(this->heapType, size);
    }
}

//------------------------------------------------------------------------------
/**
    This is the faster version to free a memory block, if the caller knows
    the size of the memory block we can compute the memory pool index
    whithout asking each pool whether the pointer is owned by this pool.
*/
void
PoolArrayAllocator::Free(void* ptr, SizeT size)
{
    IndexT poolIndex = (size + 3) >> 5;
    if (poolIndex < NumPools)
    {
        #if NEBULA_DEBUG
        n_assert(this->memoryPools[poolIndex].IsPoolBlock(ptr));
        #endif
        this->memoryPools[poolIndex].Free(ptr);
    }
    else
    {
        Memory::Free(this->heapType, ptr);
    }
}

//------------------------------------------------------------------------------
/**
    This is the slower version to free a memory block. Worst case is, that
    the allocator needs to check each memory pool whether the pointer is
    owned by the pool.
*/
void
PoolArrayAllocator::Free(void* ptr)
{
    IndexT i;
    for (i = 0; i < NumPools; i++)
    {
        if (this->memoryPools[i].IsPoolBlock(ptr))
        {
            this->memoryPools[i].Free(ptr);
            return;
        }
    }
    // fallthrough: must be a big block
    Memory::Free(this->heapType, ptr);
}

//------------------------------------------------------------------------------
/**
*/
const MemoryPool&
PoolArrayAllocator::GetMemoryPool(IndexT index) const
{
    n_assert(index < NumPools);
    return this->memoryPools[index];
}

//------------------------------------------------------------------------------
/**
*/
#if NEBULA3_MEMORY_STATS
void
PoolArrayAllocator::Dump()
{
    Util::String msg;
    msg.Format("Dump of PoolArrayAllocator '%s':\n", this->name);
    Core::SysFunc::DebugOut(msg.AsCharPtr());
    bool allOk = true;
    IndexT i;
    for (i = 0; i < NumPools; i++)
    {
        uint allocCount = this->memoryPools[i].GetAllocCount();
        if (0 != allocCount)
        {
            msg.Format("*** %d LEAKS FOR BLOCK SIZE %d!!!\n", allocCount, this->memoryPools[i].GetBlockSize());
            Core::SysFunc::DebugOut(msg.AsCharPtr());
            allOk = false;
        }
    }
    if (allOk)
    {
        Core::SysFunc::DebugOut("ALL OK!\n");
    }
}
#endif
} // namespace Memory
