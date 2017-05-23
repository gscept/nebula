//------------------------------------------------------------------------------
//  posixheap.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memory/posix/posixheap.h"

namespace Posix
{
using namespace Threading;
using namespace Util;

#if NEBULA3_MEMORY_STATS
List<PosixHeap*>* PosixHeap::list = 0;
CriticalSection* PosixHeap::criticalSection = 0;
#endif

//------------------------------------------------------------------------------
/**
    This method must be called at the beginning of the application because
    any threads are spawned (usually called by Util::Setup().
*/
void
PosixHeap::Setup()
{
    #if NEBULA3_MEMORY_STATS
    n_assert(0 == list);
    n_assert(0 == criticalSection);
    list = n_new(List<PosixHeap*>);
    criticalSection = n_new(CriticalSection);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
PosixHeap::PosixHeap(const char* heapName)
{
    n_assert(0 != heapName);
    this->name = heapName;
    
    // link into Heap list
    #if NEBULA3_MEMORY_STATS
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
PosixHeap::~PosixHeap()
{
    // unlink from Heap list
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

#if NEBULA3_MEMORY_STATS
//------------------------------------------------------------------------------
/**
    Validate the heap. This walks over the heap's memory block and checks
    the control structures. If somethings wrong the function will
    stop the program, otherwise the functions returns true.
*/
bool
PosixHeap::ValidateHeap() const
{
    BOOL result = HeapValidate(this->heap, 0, NULL);
    return (0 != result);
}


//------------------------------------------------------------------------------
/**
*/
int
PosixHeap::GetAllocCount() const
{
    return this->allocCount;
}

//------------------------------------------------------------------------------
/**
*/
int
PosixHeap::GetAllocSize() const
{
    return this->allocSize;
}

//------------------------------------------------------------------------------
/**
*/
Array<PosixHeap::Stats>
PosixHeap::GetAllHeapStats()
{
    n_assert(0 != criticalSection);
    Array<Stats> result;
    criticalSection->Enter();
    List<PosixHeap*>::Iterator iter;
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
PosixHeap::ValidateAllHeaps()
{
    n_assert(0 != criticalSection);
    criticalSection->Enter();
    bool result = true;
    List<PosixHeap*>::Iterator iter;
    for (iter = list->Begin(); iter != list->End(); iter++)
    {
        result &= (*iter)->ValidateHeap();
    }
    criticalSection->Leave();
    return result;
}
#endif // NEBULA3_MEMORY_STATS

} // namespace Memory
