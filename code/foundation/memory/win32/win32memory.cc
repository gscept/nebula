//------------------------------------------------------------------------------
//  win32memory.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "core/types.h"
#include "core/sysfunc.h"
#include "memory/heap.h"
#include "memory/poolarrayallocator.h"


namespace Memory
{
HANDLE volatile Win32ProcessHeap = 0;
long volatile TotalAllocCount = 0;
long volatile TotalAllocSize = 0;
long volatile HeapTypeAllocCount[NumHeapTypes] = { 0 };
long volatile HeapTypeAllocSize[NumHeapTypes] = { 0 };
bool volatile MemoryLoggingEnabled = false;
unsigned int volatile MemoryLoggingThreshold = 0;
HeapType volatile MemoryLoggingHeapType = InvalidHeapType;

//------------------------------------------------------------------------------
/**
    Allocate a block of memory from one of the global heaps.
*/
void*
Alloc(HeapType heapType, size_t size)
{
    n_assert(heapType < NumHeapTypes);
    
    // need to make sure everything has been setup
    Core::SysFunc::Setup();

    void* allocPtr = 0;    
    {
        n_assert(0 != Heaps[heapType]);
        allocPtr =  __HeapAlloc16(Heaps[heapType], 0, size);
        n_assert(((uintptr_t)allocPtr & 15) == 0);
        if (0 == allocPtr)
        {
            n_error("Alloc: Out of memory, allocating '%s' trying to allocate '%d' bytes\n",
                    GetHeapTypeName(heapType), size);
        }
    }
    #if NEBULA_MEMORY_STATS
        Threading::Interlocked::Increment(&TotalAllocCount);
        Threading::Interlocked::Add(&TotalAllocSize, (long)size + 16);
        Threading::Interlocked::Increment(&HeapTypeAllocCount[heapType]);
        Threading::Interlocked::Add(&HeapTypeAllocSize[heapType], (long)size + 16);
        if (MemoryLoggingEnabled && (size >= MemoryLoggingThreshold) &&
            ((MemoryLoggingHeapType == InvalidHeapType) || (MemoryLoggingHeapType == heapType)))
        {
            n_printf("Allocate(size=%d, heapType=%d): 0x%lx\n", size, heapType, (uintptr_t) allocPtr);
        }
    #endif
    return allocPtr;
}

//------------------------------------------------------------------------------
/**
    Reallocate a block of memory.
*/
void*
Realloc(HeapType heapType, void* ptr, size_t size)
{
    n_assert((heapType < NumHeapTypes) && (0 != Heaps[heapType]));
    #if NEBULA_MEMORY_STATS
        SIZE_T oldSize = __HeapSize16(Heaps[heapType], 0, ptr);
    #endif
    void* allocPtr = __HeapReAlloc16(Heaps[heapType], 0, ptr, size);
    n_assert(((uintptr_t)allocPtr & 15) == 0);
    if (0 == allocPtr)
    {
        n_error("Realloc: Out of memory, allocating '%s' trying to allocate '%d' bytes\n",
                GetHeapTypeName(heapType), size);
    }
    #if NEBULA_MEMORY_STATS
        SIZE_T newSize = __HeapSize16(Heaps[heapType], 0, allocPtr);
        Threading::Interlocked::Add(&TotalAllocSize, int(newSize - oldSize + 16));
        Threading::Interlocked::Add(&HeapTypeAllocSize[heapType], int(newSize - oldSize + 16));
        if (MemoryLoggingEnabled && (size >= MemoryLoggingThreshold) &&
            ((MemoryLoggingHeapType == InvalidHeapType) || (MemoryLoggingHeapType == heapType)))
        {
            n_printf("Reallocate(size=%d, heapType=%d): 0x%lx\n", size, heapType, (uintptr_t) allocPtr);
        }
    #endif
    return allocPtr;
}

//------------------------------------------------------------------------------
/**
    Free a chunk of memory from the process heap.
*/
void
Free(HeapType heapType, void* ptr)
{
    // D3DX on the 360 likes to call the delete operator with a 0 pointer
    if (0 != ptr)
    {
        n_assert(heapType < NumHeapTypes);
        #if NEBULA_MEMORY_STATS
            SIZE_T size = 0;
        #endif    
        n_assert(0 != Heaps[heapType]);
        #if NEBULA_MEMORY_STATS
            size = __HeapSize16(Heaps[heapType], 0, ptr);
        #endif
        __HeapFree16(Heaps[heapType], 0, ptr);
        #if NEBULA_MEMORY_STATS
            Threading::Interlocked::Add(&TotalAllocSize, -int(size));
            Threading::Interlocked::Decrement(&TotalAllocCount);
            Threading::Interlocked::Add(&HeapTypeAllocSize[heapType], -int(size));
            Threading::Interlocked::Decrement(&HeapTypeAllocCount[heapType]);
            if (MemoryLoggingEnabled && (size >= MemoryLoggingThreshold) &&
                ((MemoryLoggingHeapType == InvalidHeapType) || (MemoryLoggingHeapType == heapType)))
            {
                n_printf("Mem::Free(heapType=%d, ptr=0x%lx, allocSize=%d)\n", heapType, (uintptr_t) ptr, size);
            }
        #endif
    }
}

//------------------------------------------------------------------------------
/**
    Duplicate a 0-terminated string. The memory will be allocated from
    the StringDataHeap (important when freeing the memory!)
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
    MEMORYSTATUS stats = { NULL };
    GlobalMemoryStatus(&stats);
    TotalMemoryStatus result;
    result.totalPhysical = (unsigned int) stats.dwTotalPhys;
    result.availPhysical = (unsigned int) stats.dwAvailPhys;
    result.totalVirtual  = (unsigned int) stats.dwTotalVirtual;
    result.availVirtual  = (unsigned int) stats.dwAvailVirtual;
    return result;
}

//------------------------------------------------------------------------------
/**
    Dump detail memory status information.
*/
void
DumpTotalMemoryStatus()
{
    n_printf("DumpTotalMemoryStatus() not implemented yet in N3.\n");   
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
    Debug function which validates the process heap and all local heaps. 
    Stops the program if something is wrong. 
*/
bool
ValidateMemory()
{
    bool res = true;

    // validate global heaps
    IndexT i;
    for (i = 0; i < NumHeapTypes; i++)
    {
        if (0 != Heaps[i])
        {
            res &= (0 != HeapValidate(Heaps[i], 0, NULL));
        }
    }

    // validate local heaps
    res &= Heap::ValidateAllHeaps();

    // validate process heap
    res &= (TRUE == ::HeapValidate(::GetProcessHeap(), 0, NULL));

    return res;
}

//------------------------------------------------------------------------------
/**
    Write memory debugging info to log.
*/
void
Checkpoint(const char* msg)
{
    n_printf("MEMORY LOG: %s\n", msg);
   // ValidateMemory();

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

    // dump all Windows process heaps
    n_printf("WIN32 PROCESS HEAPS\n");
    HANDLE procHeaps[100];
    DWORD procHeapIndex;
    DWORD numProcHeaps = ::GetProcessHeaps(100, procHeaps);
    n_printf("%d PROCESS HEAPS\n", numProcHeaps);
    const HANDLE defaultHeap = ::GetProcessHeap();
    const HANDLE crtHeap = (HANDLE) _get_heap_handle();
    SizeT totalAllocSize = 0;
    SizeT totalAllocCount = 0;
    SizeT totalUnusedSize = 0;
    SizeT totalUnusedCount = 0;
    for (procHeapIndex = 0; procHeapIndex < numProcHeaps; procHeapIndex++)
    {
        // test if current heap is a Nebula heap
        const char* heapName = 0;
        IndexT nebHeapIndex;
        for (nebHeapIndex = 0; nebHeapIndex < NumHeapTypes; nebHeapIndex++)
        {
            if (Heaps[nebHeapIndex] == procHeaps[procHeapIndex])
            {
                heapName = GetHeapTypeName((HeapType)nebHeapIndex);
                break;
            }
        }

        // test if it's the CRT or standard process heap
        if (0 == heapName)
        {
            if (defaultHeap == procHeaps[procHeapIndex])
            {
                heapName = "PROCESS HEAP";
            }
            else if (crtHeap == procHeaps[procHeapIndex])
            {
                heapName = "CRT HEAP";
            }
            else
            {
                heapName = "UNKNOWN HEAP";
            }
        }

        // compute number of allocations and data size of heap
        PROCESS_HEAP_ENTRY walkEntry = { 0 };
        SizeT allocSize = 0;
        SizeT allocCount = 0;
        SizeT unusedCount = 0;
        SizeT unusedSize = 0;
        ::HeapLock(procHeaps[procHeapIndex]);
        while (::HeapWalk(procHeaps[procHeapIndex], &walkEntry))
        {
            if (walkEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
            {
                allocCount++;
                allocSize += walkEntry.cbData;
            }
            else if (walkEntry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            {
                unusedCount++;
                unusedSize += walkEntry.cbData;
            }
        }
        ::HeapUnlock(procHeaps[procHeapIndex]);
        n_printf("    heap at %lx (%s): allocCount(%d) allocSize(%d) unusedCount(%d) unusedSize(%d)\n",
            procHeaps[procHeapIndex], heapName, allocCount, allocSize, unusedCount, unusedSize);
        totalAllocSize += allocSize;
        totalAllocCount += allocCount;
        totalUnusedSize += unusedSize;
        totalUnusedCount += unusedCount;
    }
    n_printf("    PROCESS HEAPS TOTAL: allocCount(%d), allocSize(%d), unusedCount(%d), unusedSize(%d)\n", 
        totalAllocCount, totalAllocSize, totalUnusedCount, totalUnusedSize);
}

//------------------------------------------------------------------------------
/**
    Generate a memory leak dump for all heaps managed by Nebula.
*/
void
DumpMemoryLeaks()
{
    // dump heap object memory leaks
    Heap::DumpLeaksAllHeaps();

    // dump global heaps
    IndexT i;
    for (i = 0; i < NumHeapTypes; i++)
    {
        if (0 != Heaps[i])
        {
            Heap::DumpHeapMemoryLeaks(GetHeapTypeName((HeapType)i), Heaps[i]);
        }
    }
}
#endif

} // namespace Memory
