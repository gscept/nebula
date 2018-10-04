#pragma once
#ifndef POSIX_POSIXHEAP_H
#define POSIX_POSIXHEAP_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixHeap
  
    Posix implementation of the class Memory::Heap using the Posix-Heap 
    functions. Generally switches on the Low-Fragmentation-Heap, since this
    seems generally suitable for most C++ applications.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file    
*/
#include "core/types.h"
#include "threading/interlocked.h"
#include "threading/criticalsection.h"
#include "util/array.h"
#include "util/list.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixHeap
{
public:
    /// static setup method (called by Util::Setup)
    static void Setup();
    /// constructor (name must be static string!)
    PosixHeap(const char* name);
    /// destructor
    ~PosixHeap();
    /// get heap name
    const char* GetName() const;
    /// allocate a block of memory from the heap
    void* Alloc(size_t size);
    /// re-allocate a block of memory
    void* Realloc(void* ptr, size_t newSize);
    /// free a block of memory which has been allocated from this heap
    void Free(void* ptr);

    #if NEBULA_MEMORY_STATS
    /// heap stats structure
    struct Stats
    {
        const char* name;
        int allocCount;
        int allocSize;
    };
    /// gather stats from all existing heaps
    static Util::Array<Stats> GetAllHeapStats();
    /// validate all heaps
    static bool ValidateAllHeaps();
    /// validate the heap (only useful in Debug builds)
    bool ValidateHeap() const;
    /// get the current alloc count
    int GetAllocCount() const;
    /// get the current alloc size
    int GetAllocSize() const;
    #endif

private:
    /// default constructor not allowed
    PosixHeap();

    const char* name;

    #if NEBULA_MEMORY_STATS
    int volatile allocCount;
    int volatile allocSize;
    static Threading::CriticalSection*  criticalSection;
    static Util::List<PosixHeap*>* list;
    Util::List<PosixHeap*>::Iterator listIterator;
    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline const char*
PosixHeap::GetName() const
{
    n_assert(0 != this->name);
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void*
PosixHeap::Alloc(size_t size)
{
    #if NEBULA_MEMORY_STATS
    Threading::Interlocked::Increment(this->allocCount);
    Threading::Interlocked::Add(this->allocSize, int(size));
    #endif
    return malloc(size);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void*
PosixHeap::Realloc(void* ptr, size_t size)
{
    #if NEBULA_MEMORY_STATS
    size_t curSize = HeapSize(this->heap, 0, ptr);
    Threading::Interlocked::Add(this->allocSize, int(size - curSize));
    #endif
    return realloc(ptr, size);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
PosixHeap::Free(void* ptr)
{
    n_assert(0 != ptr);
    #if NEBULA_MEMORY_STATS
    size_t size = HeapSize(this->heap, 0, ptr);
    Threading::Interlocked::Add(this->allocSize, -int(size));
    Threading::Interlocked::Decrement(this->allocCount);
    #endif
    free(ptr);
}

} // namespace Posix
//------------------------------------------------------------------------------
#endif
