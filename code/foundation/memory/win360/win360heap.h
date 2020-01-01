#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360Heap
  
    Win32/Xbox360 implementation of the class Memory::Heap. Under Win32,
    the LowFragmentationHeap feature is generally turned on.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "threading/interlocked.h"
#include "threading/criticalsection.h"
#include "util/array.h"
#include "util/list.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360Heap
{
public:
    /// static setup method (called by Core::SysFunc::Setup)
    static void Setup();
    /// constructor (name must be static string!)
    Win360Heap(const char* name, size_t initialSize=0, size_t maxSize=0);
    /// destructor
    ~Win360Heap();
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
    /// dump memory leaks from this heap
    void DumpLeaks();
    /// dump memory leaks from all heaps
    static void DumpLeaksAllHeaps();
    /// get the current alloc count
    long GetAllocCount() const;
    /// get the current alloc size
    long GetAllocSize() const;
    /// helper method: generate a mem leak report for provided Windows heap
    static void DumpHeapMemoryLeaks(const char* heapName, HANDLE hHeap);
    #endif

private:
    /// default constructor not allowed
    Win360Heap();

    HANDLE heap;
    const char* name;

    #if NEBULA_MEMORY_STATS
    long volatile allocCount;
    long volatile allocSize;
    static Threading::CriticalSection*  criticalSection;
    static Util::List<Win360Heap*>* list;
    Util::List<Win360Heap*>::Iterator listIterator;
    #endif
};

//------------------------------------------------------------------------------
/**
*/
inline const char*
Win360Heap::GetName() const
{
    n_assert(0 != this->name);
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void*
Win360Heap::Alloc(size_t size)
{
    #if NEBULA_MEMORY_STATS
    Threading::Interlocked::Increment(this->allocCount);
	// __HeapAlloc16 will always add 16 bytes for memory alignment padding
    Threading::Interlocked::Add(this->allocSize, int(size + 16));
    #endif
    void* ptr = Memory::__HeapAlloc16(this->heap, HEAP_GENERATE_EXCEPTIONS, size);
    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void*
Win360Heap::Realloc(void* ptr, size_t size)
{
    #if NEBULA_MEMORY_STATS
    size_t curSize = Memory::__HeapSize16(this->heap, 0, ptr);
	// __HeapAlloc16 will always add 16 bytes for memory alignment padding
	Threading::Interlocked::Add(this->allocSize, int(size - curSize + 16));
    #endif
    void* newPtr = Memory::__HeapReAlloc16(this->heap, HEAP_GENERATE_EXCEPTIONS, ptr, size);
    return newPtr;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
Win360Heap::Free(void* ptr)
{
    n_assert(0 != ptr);
    #if NEBULA_MEMORY_STATS
    size_t size = Memory::__HeapSize16(this->heap, 0, ptr);
    Threading::Interlocked::Add(this->allocSize, -int(size));
    Threading::Interlocked::Decrement(this->allocCount);
    #endif
    BOOL success = Memory::__HeapFree16(this->heap, 0, ptr);
    n_assert(0 != success);
}

} // namespace Win360Heap
//------------------------------------------------------------------------------
