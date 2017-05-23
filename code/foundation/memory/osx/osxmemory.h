#pragma once
//------------------------------------------------------------------------------
/**
    @file memory/osx/osxmemory.h
 
    Lowlevel memory functions for the OSX platform.
 
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file    
 */
#include "core/config.h"
#include "core/debug.h"
#include "memory/osx/osxmemoryconfig.h"

namespace Memory
{
extern int volatile TotalAllocCount;
extern int volatile TotalAllocSize;
extern int volatile HeapTypeAllocCount[NumHeapTypes];
extern int volatile HeapTypeAllocSize[NumHeapTypes];
extern bool volatile MemoryLoggingEnabled;
extern unsigned int volatile MemoryLoggingThreshold;
extern HeapType volatile MemoryLoogingHeapType;
    
//------------------------------------------------------------------------------
/**
    Global memory functions.
 */
/// allocate a chunk of memory
extern void* Alloc(HeapType heapType, size_t size, size_t alignment=16);
/// re-allocate a chunk of memory
extern void* Realloc(HeapType heapType, void* ptr, size_t size);
/// free a chunk of memory
extern void Free(HeapType heapType, void* ptr);
/// duplicate a C-string (obsolete)
extern char* DuplicateCString(const char* from);
/// check if 2 memory regions are overlapping
extern bool IsOverlapping(const unsigned char* srcPtr, size_t srcSize, const unsigned char* dstPtr, size_t dstSize);
/// copy a chunk of memory
extern void Copy(const void* from, void* to, size_t numBytes);
/// overwrite a chunk of memory with zero
extern void Clear(void* ptr, size_t numBytes);
/// fill memory with a specific byte
extern void Fill(void* ptr, size_t numBytes, unsigned char value);
    
//------------------------------------------------------------------------------
/**
    Get the system's total current memory, this does not only include
    Nebula3's memory allocations but the memory usage of the entire system.
*/
struct TotalMemoryStatus
{
    unsigned int totalPhysical;
    unsigned int availPhysical;
    unsigned int totalVirtual;
    unsigned int availVirtual;
};
extern TotalMemoryStatus GetTotalMemoryStatus();
    
//------------------------------------------------------------------------------
/**
    Debug and memory validation functions.
*/
#if NEBULA3_MEMORY_STATS
/// enable memory logging
void EnableMemoryLogging(unsigned int threshold, HeapType = InvalidHeapType);
/// disable memory logging
void DisableMemoryLogging();
/// toggle memory logging
void ToggleMemoryLogging(unsigned int threshold, HeapType = InvalidHeapType);
#endif
    
#if NEBULA3_MEMORY_ADVANCED_DEBUGGING
/// check memory lists for consistency
extern bool ValidateMemory();
/// dump current memory status to log file
extern void Checkpoint(const char* msg);
/// dump memory leaks
void DumpMemoryLeaks();
#endif
    
#if NEBULA3_MEMORY_ADVANCED_DEBUGGING
#define __MEMORY_CHECKPOINT(s) Memory::Checkpoint(##s)
#else
#define __MEMORY_CHECKPOINT(s)
#endif
    
    // FIXME: Memory-Validation disabled for now
#define __MEMORY_VALIDATE(s)
} // namespace Memory

#ifdef new
#undef new
#endif

#ifdef delete
#undef delete
#endif

//------------------------------------------------------------------------------
/**
 Replacement global new/delete operators.
 */
extern void* operator new(size_t size);
extern void* operator new(size_t size, size_t align);

extern void* operator new[](size_t size);
extern void* operator new[](size_t size, size_t align);

extern void operator delete(void* p);
extern void operator delete[](void* p);

#define n_new(type) new type
#define n_new_array(type,size) new type[size]
#define n_delete(ptr) delete ptr
#define n_delete_array(ptr) delete[] ptr
//------------------------------------------------------------------------------


