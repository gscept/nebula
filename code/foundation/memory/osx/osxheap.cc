//------------------------------------------------------------------------------
//  osxheap.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memory/osx/osxheap.h"
#include "core/sysfunc.h"

namespace OSX
{

#if NEBULA3_MEMORY_STATS
List<OSXHeap*>* OSXHeap::list = 0;
CriticalSection* OSXHeap::criticalSection = 0;
#endif

//------------------------------------------------------------------------------
/**
    This method must be called at the beginning of the application before
    any threads are spawned.
 */
void
OSXHeap::Setup()
{
    #if NEBULA3_MEMORY_STATS
    n_assert(0 == list);
    n_assert(0 == criticalSection);
    list = n_new(List<OSXHeap*>);
    criticalSection = n_new(CriticalSection);
    #endif
}
    
//------------------------------------------------------------------------------
/**
*/
OSXHeap::OSXHeap(const char* heapName, size_t initialSize)
{
    n_assert(0 != heapName);
    this->name = heapName;
    this->heapZone = malloc_create_zone(initialSize, 0);
    
    #if NEBULA3_MEMORY_STATS
    n_assert(0 != criticalSection);
    this->allocCount = 0;
    this->allocSize = 0;
    criticalSection->Enter();
    this->listIterator = list->AddBack(this);
    criticalSection->Leave();
    #endif
}

//------------------------------------------------------------------------------
/**
*/
OSXHeap::~OSXHeap()
{
    #if NEBULA3_MEMORY_STATS
    this->DumpLeaks();
    #endif
    
    malloc_destroy_zone(this->heapZone);
    this->heapZone = 0;
    
    // dump memory leaks and unlink from heap list
    #if NEBULA3_MEMORY_STATS
    n_assert(0 == this->allocCount);
    n_assert(0 != criticalSection);
    n_assert(0 != this->listIterator);
    criticalSection->Enter();
    list->Remove(this->listIterator);
    criticalSection->Leave();
    this->listIterator = 0;
    #endif   
}

//------------------------------------------------------------------------------
/**
*/
void*
OSXHeap::Alloc(size_t size, size_t alignment)
{
    void* allocPtr = malloc_zone_memalign(this->heapZone, alignment, size);
    if (0 == allocPtr)
    {
        n_error("OSXHeap::Alloc(): Out of mem on heap '%s', trying to allocate %i bytes!\n", this->name, int(size));
    }
    #if NEBULA3_MEMORY_STATS
    size_t allocatedSize = malloc_size(allocPtr);
    Threading::Interlocked::Increment(this->allocCount);
    Threading::Interlocked::Add(this->allocSize, allocatedSize);
    #endif
    return allocPtr;
}
        
//------------------------------------------------------------------------------
/**
*/
void*
OSXHeap::Realloc(void* ptr, size_t size)
{
    #if NEBULA3_MEMORY_STATS
    size_t oldSize = malloc_size(ptr);
    #endif
    void* allocPtr = malloc_zone_realloc(this->heapZone, ptr, size);
    if (0 == allocPtr)
    {
        n_error("OSXHeap::Realloc(): Out of memory on heap '%s', trying to re-alloc %i bytes!\n", this->name, (int)size);
    }
    #if NEBULA3_MEMORY_STATS
    size_t newSize = malloc_size(ptr);
    Threading::Interlocked::Add(this->allocSize, int(newSize - oldSize));
    #endif
    return allocPtr;
}

//------------------------------------------------------------------------------
/**
*/
void
OSXHeap::Free(void* ptr)
{
    n_assert(0 != ptr);
    #if NEBULA3_MEMORY_STATS
    size_t allocSize = malloc_size(ptr);
    Threading::Interlocked::Add(this->allocSize, -int(allocSize));
    Threading::Interlocked::Decrement(this->allocCount);
    #endif
    malloc_zone_free(this->heapZone, ptr);
}

#if NEBULA3_MEMORY_STATS
//------------------------------------------------------------------------------
/**
    Validate the heap. This walks over the heap's memory block and checks
    the control structures. If somethings wrong the function will
    stop the program, otherwise the functions returns true.
*/
bool
OSXHeap::ValidateHeap() const
{
    return malloc_zone_check(this->heapZone);
}

//------------------------------------------------------------------------------
/**
*/
int
OSXHeap::GetAllocCount() const
{
    return this->allocCount;
}
    
//------------------------------------------------------------------------------
/**
*/
int
OSXHeap::GetAllocSize() const
{
    return this->allocSize;
}
    
//------------------------------------------------------------------------------
/**
*/
Array<OSXHeap::Stats>
OSXHeap::GetAllHeapStats()
{
    n_assert(0 != criticalSection);
    Array<Stats> result;
    criticalSection->Enter();
    List<OSXHeap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        Stats stats;
        stats.name = (*iter)->GetName();
        stats.allocCount = (*iter)->GetAllocCount();
        stats.allocSize = (*iter)->GetAllocSize();
        result.Append(stats);
    }
    criticalSection->Leave();
    return result
}
        
//------------------------------------------------------------------------------
/**
    This static method calls the ValidateHeap() method on all heaps.
*/
bool
OSXHeap::ValidateAllHeaps()
{
    n_assert(0 != criticalSection);
    criticalSection->Enter();
    bool result = true;
    List<OSXHeap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        result &= (*iter)->ValidateHeap();
    }
    criticalSection->Leave();
    return result;
}
    
//------------------------------------------------------------------------------
/**
    Generates a memory leak report for this heap.
*/
void
OSXHeap::DumpLeaks()
{
    Core::SysFunc::Error("OSXHeap::DumpLeaks() not implemented!\n");
}

//------------------------------------------------------------------------------
/**
    Generates memory leaks for every heap object.
*/
void
OSXHeap::DumpLeaksAllHeaps()
{
    n_assert(0 != criticalSection);
    criticalSection->Enter();
    List<OSXHeap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        (*iter)->DumpLeaks();
    }
    criticalSection->Leave();
}
#endif // NEBULA3_MEMORY_STATS

} // namespace Memory