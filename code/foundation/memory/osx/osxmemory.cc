//------------------------------------------------------------------------------
//  osxmemory.cc
//  (C) 2010 RadonLabs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/types.h"
#include "core/sysfunc.h"

// #include "threading/interlocked.h"

// overriden new/delete operators
void* operator new(size_t size)
{
    return Memory::Alloc(Memory::ObjectHeap, size);
}
void* operator new(size_t size, size_t align)
{
    return Memory::Alloc(Memory::ObjectHeap, size, align);
}
void* operator new[](size_t size)
{
    return Memory::Alloc(Memory::ObjectArrayHeap, size);
}
void* operator new[](size_t size, size_t align)
{
    return Memory::Alloc(Memory::ObjectArrayHeap, size, align);
}
void operator delete(void* p)
{
    Memory::Free(Memory::ObjectHeap, p);
}
void operator delete[](void* p)
{
    Memory::Free(Memory::ObjectArrayHeap, p);
}

namespace Memory
{
    
#if NEBULA_MEMORY_STATS
int volatile TotalAllocCount = 0;
int volatile TotalAllocSize = 0;
int volatile HeapTypeAllocCount[NumHeapTypes] = { 0 };
int volatile HeapTypeAllocSize[NumHeapTypes] = { 0 };
bool volatile MemoryLoggingEnabled = false;
unsigned int volatile MemoryLoggingThreshold = 0;
HeapType volatile MemoryLoggingHeapType = InvalidHeapType;
#endif
    
//------------------------------------------------------------------------------
/**
    Allocate a block of memory from one of the global heaps.
*/
void*
Alloc(HeapType heapType, size_t size, size_t alignment)
{
    n_assert(heapType < NumHeapTypes);
    
    // make sure everything has been setup already
    Core::SysFunc::Setup();
        
    void* allocPtr = 0;
#if NEBULA_MEMORY_STATS
    size_t allocatedSize = 0;
#endif
        
    // allocate memory from global heap    
    allocPtr = malloc_zone_memalign(Heaps[heapType], alignment, size);
    if (0 == allocPtr)
    {
        n_error("Allocation failed from Heap '%s'!\n", GetHeapTypeName(heapType));
    }
#if NEBULA_MEMORY_STATS
    allocatedSize = malloc_size(allocPtr);
#endif
        
#if NEBULA_MEMORY_STATS                
    Threading::Interlocked::Increment(TotalAllocCount);
    Threading::Interlocked::Add(TotalAllocSize, allocatedSize);
    Threading::Interlocked::Increment(HeapTypeAllocCount[heapType]);
    Threading::Interlocked::Add(HeapTypeAllocSize[heapType], allocatedSize);
    if (MemoryLoggingEnabled && (size >= MemoryLoggingThreshold) &&
        ((MemoryLoggingHeapType == InvalidHeapType) || (MemoryLoggingHeapType == heapType)))
    {
        n_printf("Allocate(size=%d, allocSize=%d, heapType=%d): 0x%lx\n", size, allocatedSize, heapType, (long unsigned int) allocPtr);
    }
#endif
    return allocPtr;
}
    
//------------------------------------------------------------------------------
/**
    Re-Allocate a block of memory from one of the global heaps.
 
    NOTE that this function may also be used to shrink a memory block!
*/
void*
Realloc(HeapType heapType, void* ptr, size_t size)
{
    n_assert(heapType < NumHeapTypes);
    
    // make sure everything has been setup already
    Core::SysFunc::Setup();
                
    // get old size for stats tracking
#if NEBULA_MEMORY_STATS
    size_t oldSize = malloc_size(ptr);
#endif
        
    // reallocate the memory
    void* allocPtr = malloc_zone_realloc(Heaps[heapType], ptr, size);
    if (0 == allocPtr)
    {
        n_error("Allocation failed from Heap '%s'!\n", GetHeapTypeName(heapType));
    }
        
#if NEBULA_MEMORY_STATS
    size_t allocatedSize = mspace_malloc_usable_size(allocPtr);
    Threading::Interlocked::Add(TotalAllocSize, size_t(allocatedSize - oldSize));
    Threading::Interlocked::Add(HeapTypeAllocSize[heapType], size_t(allocatedSize - oldSize));
    if (MemoryLoggingEnabled && (size >= MemoryLoggingThreshold) &&
        ((MemoryLoggingHeapType == InvalidHeapType) || (MemoryLoggingHeapType == heapType)))
    {
        n_printf("Reallocate(size=%d, allocSize=%d, heapType=%d): 0x%lx\n", size, allocatedSize, heapType, (long unsigned int) allocPtr);
    }
#endif
    return allocPtr;
}
    
//------------------------------------------------------------------------------
/**
    Free a block of memory.
*/
void
Free(HeapType heapType, void* ptr)
{
    if (0 != ptr)
    {
        n_assert(heapType < NumHeapTypes);
            
#if NEBULA_MEMORY_STATS
        size_t allocatedSize = malloc_size(ptr);
#endif
        malloc_zone_free(Heaps[heapType], ptr);
            
#if NEBULA_MEMORY_STATS
        Threading::Interlocked::Add(TotalAllocSize, -allocatedSize);
        Threading::Interlocked::Decrement(TotalAllocCount);
        Threading::Interlocked::Add(HeapTypeAllocSize[heapType], -allocatedSize);
        Threading::Interlocked::Decrement(HeapTypeAllocCount[heapType]);
        if (MemoryLoggingEnabled && (allocatedSize >= MemoryLoggingThreshold) &&
            ((MemoryLoggingHeapType == InvalidHeapType) || (MemoryLoggingHeapType == heapType)))
        {
            n_printf("Mem::Free(heapType=%d, ptr=0x%lx, allocSize=%d)\n", heapType, (long unsigned int) ptr, allocatedSize);
        }
#endif
    }
}
    
//------------------------------------------------------------------------------
/**
    Duplicate a 0-terminated string, this method should no longer be used!
*/
char*
DuplicateCString(const char* from)
{
    n_assert(0 != from);
    size_t len = (unsigned int) strlen(from) + 1;
    char* to = (char*) Memory::Alloc(Memory::StringDataHeap, len);
    Memory::Copy((void*)from, to, len);
    return to;
}
    
//------------------------------------------------------------------------------
/**
    Test if 2 areas of memory areas are overlapping.
*/
bool
IsOverlapping(const unsigned char* srcPtr, size_t srcSize, const unsigned char* dstPtr, size_t dstSize)
{
    if (srcPtr == dstPtr)
    {
        return true;
    }
    else if (srcPtr > dstPtr)
    {
        return (srcPtr + srcSize) > dstPtr;
    }
    else
    {
        return (dstPtr + dstSize) > srcPtr;
    }
}
    
//------------------------------------------------------------------------------
/**
    Get the system's total memory status.
*/
TotalMemoryStatus
GetTotalMemoryStatus()
{
    n_error("IMPLEMENT ME: GetTotalMemoryStatus()!");
    TotalMemoryStatus status = { 0 };
    return status;
}
    
//------------------------------------------------------------------------------
/**
    Copy a chunk of memory (note the argument order is different from memcpy()!!!)
*/
void
Copy(const void* from, void* to, size_t numBytes)
{
    if (numBytes > 0)
    {
        n_assert(0 != from);
        n_assert(0 != to);
        n_assert(from != to);
        memcpy(to, from, numBytes);
    }
}
    
//------------------------------------------------------------------------------
/**
    Overwrite a chunk of memory with 0's.
*/
void
Clear(void* ptr, size_t numBytes)
{
    memset(ptr, 0, numBytes);
}
    
//------------------------------------------------------------------------------
/**
    Fill memory with a specific byte.
*/
void
Fill(void* ptr, size_t numBytes, unsigned char value)
{
    memset(ptr, value, numBytes);
}
    
#if NEBULA_MEMORY_STATS
//------------------------------------------------------------------------------
/**
    Enable memory logging.
*/
void
EnableMemoryLogging(unsigned int threshold, HeapType heapType)
{
    MemoryLoggingEnabled = true;
    MemoryLoggingThreshold = threshold;
    MemoryLoggingHeapType = heapType;
}
    
//------------------------------------------------------------------------------
/**
    Disable memory logging.
*/
void
DisableMemoryLogging()
{
    MemoryLoggingEnabled = false;
}
    
//------------------------------------------------------------------------------
/**
    Toggle memory logging.
*/
void
ToggleMemoryLogging(unsigned int threshold, HeapType heapType)
{
    if (MemoryLoggingEnabled)
    {
        DisableMemoryLogging();
    }
    else
    {
        EnableMemoryLogging(threshold, heapType);
    }
}
#endif // NEBULA_MEMORY_STATS
    
#if NEBULA_MEMORY_ADVANCED_DEBUGGING
//------------------------------------------------------------------------------
/**
    Debug function which validates all heaps. 
*/
bool
ValidateMemory()
{
    n_error("IMPLEMENT ME: ValidateMemory()");
    return true;
}
    
//------------------------------------------------------------------------------
/**
    Write memory debugging info to log.
*/
void
Checkpoint(const char* msg)
{
    n_printf("MEMORY LOG: %s\n", msg);
    ValidateMemory();
    
    // also dump a general alloc count/alloc size by heap type...
    n_printf("NEBULA ALLOC COUNT / SIZE: %d / %d\n", TotalAllocCount, TotalAllocSize);
    IndexT i;
    for (i = 0; i < NumHeapTypes; i++)
    {
        const char* heapName = GetHeapTypeName((HeapType)i);
        if (0 == heapName)
        {
            heapName = "UNKNOWN";
        }
        n_printf("HEAP %lx ALLOC COUNT / SIZE: %s %d / %d\n", Heaps[i], heapName, HeapTypeAllocCount[i], HeapTypeAllocSize[i]);
    }
}
    
//------------------------------------------------------------------------------
/**
    Generate a memory leak dump for all heaps managed by Nebula.
*/
void
DumpMemoryLeaks()
{
    n_error("IMPLEMENT ME: DumpMemoryLeaks()");
}
#endif // NEBULA_MEMORY_ADVANCED_DEBUGGING
    
} // namespace Memory
