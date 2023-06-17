#pragma once
#ifndef MEMORY_POSIXMEMORY_H
#define MEMORY_POSIXMEMORY_H
//------------------------------------------------------------------------------
/**
    @file memory/posix/posixmemory.h
    
    Memory subsystem features for the Posix platform
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file    
*/
#include "core/config.h"
#include "core/debug.h"
#include "threading/interlocked.h"
#include "memory/posix/posixmemoryconfig.h"
#include <malloc.h>
#include <string.h>
#include <sys/mman.h>

namespace Memory
{
#if NEBULA_MEMORY_STATS
extern int volatile TotalAllocCount;
extern int volatile TotalAllocSize;
extern int volatile HeapTypeAllocCount[NumHeapTypes];
extern int volatile HeapTypeAllocSize[NumHeapTypes];
#endif

//------------------------------------------------------------------------------
/**
    Allocate a block of memory from the process heap.
*/
__forceinline void*
Alloc(HeapType heapType, size_t size, size_t align = 16)
{
    n_assert(heapType < NumHeapTypes);
    void* allocPtr = 0;
    {
        int err = posix_memalign(&allocPtr, align, size);
        n_assert(err == 0);
        #if NEBULA_DEBUG
        explicit_bzero(allocPtr,size);
        #endif
    }
    #if NEBULA_MEMORY_STATS
        SIZE_T s = HeapSize(Heaps[heapType], 0, allocPtr);
        Threading::Interlocked::Increment(TotalAllocCount);
        Threading::Interlocked::Add(TotalAllocSize, int(s));
        Threading::Interlocked::Increment(HeapTypeAllocCount[heapType]);
        Threading::Interlocked::Add(HeapTypeAllocSize[heapType], int(s));
    #endif
    return allocPtr;
}

//------------------------------------------------------------------------------
/**
    Reallocate a block of memory.
*/
__forceinline void*
Realloc(HeapType heapType, void* ptr, size_t size)
{
    n_assert(heapType < NumHeapTypes);
    #if NEBULA_MEMORY_STATS
        SIZE_T oldSize = HeapSize(Heaps[heapType], 0, ptr);
    #endif
    void* allocPtr = realloc(ptr, size);
    #if NEBULA_MEMORY_STATS
        SIZE_T newSize = HeapSize(Heaps[heapType], 0, allocPtr);
        Threading::Interlocked::Add(TotalAllocSize, int(newSize - oldSize));
        Threading::Interlocked::Add(HeapTypeAllocSize[heapType], int(newSize - oldSize));
    #endif
    return allocPtr;
}

//------------------------------------------------------------------------------
/**
    Free a chunk of memory from the process heap.
*/
__forceinline void
Free(HeapType heapType, void* ptr)
{
    // D3DX on the 360 likes to call the delete operator with a 0 pointer
    if (0 != ptr)
    {
        n_assert(heapType < NumHeapTypes);
        #if NEBULA_MEMORY_STATS
            SIZE_T size = 0;
        #endif    
        {
            #if NEBULA_MEMORY_STATS
                size = HeapSize(Heaps[heapType], 0, ptr);
            #endif
            free(ptr);
        }
        #if NEBULA_MEMORY_STATS
            Threading::Interlocked::Add(TotalAllocSize, -int(size));
            Threading::Interlocked::Decrement(TotalAllocCount);
            Threading::Interlocked::Add(HeapTypeAllocSize[heapType], -int(size));
            Threading::Interlocked::Decrement(HeapTypeAllocCount[heapType]);
        #endif
    }
}

//------------------------------------------------------------------------------
/**
*/
void* 
AllocVirtual(size_t size)
{
    void* ret = mmap(nullptr, reservationSize, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
    n_assert(ret != nullptr);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
CommitVirtual(void* ptr, size_t size)
{
    auto ret = mprotect(ptr, size, PROT_READ | PROT_WRITE);
    n_assert(ret == 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
FreeVirtual(void* ptr, size_t size)
{
    auto ret = munmap(ptr, size);
    n_assert(ret == 0);
    ret = madvise(ptr, size, MADV_DONTNEED);
    n_assert(ret == 0);
}

//------------------------------------------------------------------------------
/**
    Copy a chunk of memory (note the argument order is different from memcpy()!!!)
*/
__forceinline void
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
    Move a chunk of memory, can handle overlapping regions
*/
__forceinline void
Move(const void* from, void* to, size_t numBytes)
{
    if (numBytes > 0)
    {
        n_assert(0 != from);
        n_assert(0 != to);
        n_assert(from != to);
        memmove(to, from, numBytes);
    }
}

//------------------------------------------------------------------------------
/**
    Move a chunk of memory, can handle overlapping regions
*/
template <typename T>
__forceinline void
MoveElements(const T* from, T* to, size_t numElements)
{
    if (numElements > 0)
    {
        n_assert(0 != from);
        n_assert(0 != to);
        n_assert(from != to);
        memmove((void*)to, (const void*)from, numElements * sizeof(T));
    }
}

//------------------------------------------------------------------------------
/**
    Copy a chunk of memory (note the argument order is different from memcpy()!!!)
*/
template <typename T>
__forceinline void
CopyElements(const T* from, T* to, size_t numElements)
{
    if (numElements > 0)
    {
        n_assert(0 != from);
        n_assert(0 != to);
        n_assert(from != to);
        memcpy(to, from, numElements * sizeof(T));
    }
}


//------------------------------------------------------------------------------
/**
    Copy data from a system memory buffer to graphics resource memory. Some
    platforms may need special handling of this case.
*/
__forceinline void
CopyToGraphicsMemory(const void* from, void* to, size_t numBytes)
{
    // no special handling on the Win32 platform
    Memory::Copy(from, to, numBytes);
}

//------------------------------------------------------------------------------
/**
    Overwrite a chunk of memory with 0's.
*/
__forceinline void
Clear(void* ptr, size_t numBytes)
{
    memset(ptr, 0, numBytes);
}

//------------------------------------------------------------------------------
/**
    Fill memory with a specific byte.
*/
__forceinline void
Fill(void* ptr, size_t numBytes, unsigned char value)
{
    memset(ptr, value, numBytes);
}

//------------------------------------------------------------------------------
/**
    Duplicate a 0-terminated string. The memory will be allocated from
    the StringHeap (important when freeing the memory!)
*/
__forceinline char*
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
    Test if 2 areas of memory area overlapping.
*/
inline bool
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
    Get the system's total current memory, this does not only include
    Nebula's memory allocations but the memory usage of the entire system.
*/
struct TotalMemoryStatus
{
    unsigned int totalPhysical;
    unsigned int availPhysical;
    unsigned int totalVirtual;
    unsigned int availVirtual;
};

inline TotalMemoryStatus
GetTotalMemoryStatus()
{
#if 0
    MEMORYSTATUS stats = { NULL };
    GlobalMemoryStatus(&stats);
    TotalMemoryStatus result;
    result.totalPhysical = (unsigned int) stats.dwTotalPhys;
    result.availPhysical = (unsigned int) stats.dwAvailPhys;
    result.totalVirtual  = (unsigned int) stats.dwTotalVirtual;
    result.availVirtual  = (unsigned int) stats.dwAvailVirtual;
    return result;
#else
    TotalMemoryStatus result;
    return result;
#endif
}

//------------------------------------------------------------------------------
/**
    Debug function which validates the process heap. This will NOT check
    local heaps (call Heap::ValidateAllHeaps() for this). 
    Stops the program if something is wrong. 
*/
#if NEBULA_MEMORY_STATS
extern bool Validate();
#endif

} // namespace Memory
//------------------------------------------------------------------------------
#endif

