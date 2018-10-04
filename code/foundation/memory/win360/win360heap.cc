//------------------------------------------------------------------------------
//  win360heap.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "memory/win360/win360heap.h"

namespace Win360
{
using namespace Threading;
using namespace Util;

#if NEBULA_MEMORY_STATS
List<Win360Heap*>* Win360Heap::list = 0;
CriticalSection* Win360Heap::criticalSection = 0;
#endif

//------------------------------------------------------------------------------
/**
    This method must be called at the beginning of the application before
    any threads are spawned.
*/
void
Win360Heap::Setup()
{
    #if NEBULA_MEMORY_STATS
    n_assert(0 == list);
    n_assert(0 == criticalSection);
    list = n_new(List<Win360Heap*>);
    criticalSection = n_new(CriticalSection);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
Win360Heap::Win360Heap(const char* heapName, size_t initialSize, size_t maxSize)
{
    n_assert(0 != heapName);
    this->name = heapName;
    this->heap = ::HeapCreate(0, initialSize, maxSize);
    n_assert(0 != this->heap);
    
    // enable low-fragmentatio-heap (Win32 only)
    #if __WIN32__
    ULONG heapFragValue = 2;
    HeapSetInformation(this->heap, HeapCompatibilityInformation, &heapFragValue, sizeof(heapFragValue));
    #endif

    // link into Heap list
    #if NEBULA_MEMORY_STATS
    n_assert(0 != criticalSection);
    this->allocCount = 0;
    this->allocSize  = 0;
    criticalSection->Enter();
    this->listIterator = list->AddBack(this);
    criticalSection->Leave();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
Win360Heap::~Win360Heap()
{
    #if NEBULA_MEMORY_STATS
    this->DumpLeaks();
    #endif

    BOOL success = ::HeapDestroy(this->heap);
    n_assert(0 != success);
    this->heap = 0;

    // dump memory leaks and unlink from Heap list
    #if NEBULA_MEMORY_STATS
    n_assert(0 == this->allocCount);
    n_assert(0 != criticalSection);
    n_assert(0 != this->listIterator);
    criticalSection->Enter();
    list->Remove(this->listIterator);
    criticalSection->Leave();
    this->listIterator = 0;
    #endif   
}

#if NEBULA_MEMORY_STATS
//------------------------------------------------------------------------------
/**
    Validate the heap. This walks over the heap's memory block and checks
    the control structures. If somethings wrong the function will
    stop the program, otherwise the functions returns true.
*/
bool
Win360Heap::ValidateHeap() const
{
    BOOL result = ::HeapValidate(this->heap, 0, NULL);
    return (0 != result);
}

//------------------------------------------------------------------------------
/**
*/
long
Win360Heap::GetAllocCount() const
{
    return this->allocCount;
}

//------------------------------------------------------------------------------
/**
*/
long
Win360Heap::GetAllocSize() const
{
    return this->allocSize;
}

//------------------------------------------------------------------------------
/**
*/
Array<Win360Heap::Stats>
Win360Heap::GetAllHeapStats()
{
    n_assert(0 != criticalSection);
    Array<Stats> result;
    criticalSection->Enter();
    List<Win360Heap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        Stats stats;
        stats.name       = (*iter)->GetName();
        stats.allocCount = (*iter)->GetAllocCount();
        stats.allocSize  = (*iter)->GetAllocSize();        
        result.Append(stats);
    }
    criticalSection->Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
    This static method calls the ValidateHeap() method on all heaps.
*/
bool
Win360Heap::ValidateAllHeaps()
{
    n_assert(0 != criticalSection);
    criticalSection->Enter();
    bool result = true;
    List<Win360Heap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        result &= (*iter)->ValidateHeap();
    }
    criticalSection->Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
    This is a helper method which walks a Windows heap and writes a
    log entry per allocated memory block to DebugOut.
*/
void
Win360Heap::DumpHeapMemoryLeaks(const char* heapName, HANDLE hHeap)
{
    PROCESS_HEAP_ENTRY walkEntry = { 0 };
    ::HeapLock(hHeap);
    bool heapMsgShown = false;
    while (::HeapWalk(hHeap, &walkEntry))
    {
        if (walkEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
        {
            char strBuf[256];
            if (!heapMsgShown)
            {
                _snprintf(strBuf, sizeof(strBuf), "!!! HEAP MEMORY LEAKS !!! (Heap: %s)\n", heapName);
                ::OutputDebugString(strBuf);
                heapMsgShown = true;
            }
            _snprintf(strBuf, sizeof(strBuf), "addr(0x%0zx) size(%d)\n", (size_t) walkEntry.lpData, walkEntry.cbData);
            ::OutputDebugString(strBuf);
        }
    }
    ::HeapUnlock(hHeap);
}

//------------------------------------------------------------------------------
/**
    Generates a memory leak report for this heap.
*/
void
Win360Heap::DumpLeaks()
{
    n_assert(0 != this->name);
    n_assert(0 != this->heap);
    Win360Heap::DumpHeapMemoryLeaks(this->name, this->heap);
}

//------------------------------------------------------------------------------
/**
    Generates memory leaks for every heap object.
*/
void
Win360Heap::DumpLeaksAllHeaps()
{
    n_assert(0 != criticalSection);
    criticalSection->Enter();
    List<Win360Heap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        (*iter)->DumpLeaks();
    }
    criticalSection->Leave();
}
#endif // NEBULA_MEMORY_ADVANCED_DEBUGGING

} // namespace Memory
