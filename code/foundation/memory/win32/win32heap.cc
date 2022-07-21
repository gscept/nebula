//------------------------------------------------------------------------------
//  win32heap.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "memory/win32/win32heap.h"

namespace Win32
{
using namespace Threading;
using namespace Util;

#if NEBULA_MEMORY_STATS
List<Win32Heap*>* Win32Heap::list = 0;
CriticalSection* Win32Heap::criticalSection = 0;
#endif

//------------------------------------------------------------------------------
/**
    This method must be called at the beginning of the application before
    any threads are spawned.
*/
void
Win32Heap::Setup()
{
    #if NEBULA_MEMORY_STATS
    n_assert(0 == list);
    n_assert(0 == criticalSection);
    list = n_new(List<Win32Heap*>);
    criticalSection = n_new(CriticalSection);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
Win32Heap::Win32Heap(const char* heapName, size_t initialSize, size_t maxSize)
{
    n_assert(0 != heapName);
    this->name = heapName;
    this->heap = ::HeapCreate(0, initialSize, maxSize);
    n_assert(0 != this->heap);
    
    // enable low-fragmentatio-heap (Win32 only)
    #if __WIN32__
    ULONG heapFragValue = 2;
    HeapSetInformation(this->heap, HeapCompatibilityInformation, &heapFragValue, sizeof(heapFragValue));
    #if NEBULA_MEMORY_ADVANCED_DEBUGGING
    HeapSetInformation(this->heap, HeapEnableTerminationOnCorruption, nullptr, 0);
    #endif
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
Win32Heap::~Win32Heap()
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
Win32Heap::ValidateHeap() const
{
    BOOL result = ::HeapValidate(this->heap, 0, NULL);
    return (0 != result);
}

//------------------------------------------------------------------------------
/**
*/
long
Win32Heap::GetAllocCount() const
{
    return this->allocCount;
}

//------------------------------------------------------------------------------
/**
*/
long
Win32Heap::GetAllocSize() const
{
    return this->allocSize;
}

//------------------------------------------------------------------------------
/**
*/
Array<Win32Heap::Stats>
Win32Heap::GetAllHeapStats()
{
    n_assert(0 != criticalSection);
    Array<Stats> result;
    criticalSection->Enter();
    List<Win32Heap*>::Iterator iter;
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
Win32Heap::ValidateAllHeaps()
{
    n_assert(0 != criticalSection);
    criticalSection->Enter();
    bool result = true;
    List<Win32Heap*>::Iterator iter;
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
Win32Heap::DumpHeapMemoryLeaks(const char* heapName, HANDLE hHeap)
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
Win32Heap::DumpLeaks()
{
    n_assert(0 != this->name);
    n_assert(0 != this->heap);
    Win32Heap::DumpHeapMemoryLeaks(this->name, this->heap);
}

//------------------------------------------------------------------------------
/**
    Generates memory leaks for every heap object.
*/
void
Win32Heap::DumpLeaksAllHeaps()
{
    n_assert(0 != criticalSection);
    criticalSection->Enter();
    List<Win32Heap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        (*iter)->DumpLeaks();
    }
    criticalSection->Leave();
}
#endif // NEBULA_MEMORY_ADVANCED_DEBUGGING

} // namespace Memory
