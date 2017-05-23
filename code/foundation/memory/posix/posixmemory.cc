//------------------------------------------------------------------------------
//  win360memory.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/types.h"
#include "memory/heap.h"

namespace Memory
{
void* volatile PosixProcessHeap = 0;
#if NEBULA3_MEMORY_STATS
int volatile TotalAllocCount = 0;
int volatile TotalAllocSize = 0;
int volatile HeapTypeAllocCount[NumHeapTypes] = { 0 };
int volatile HeapTypeAllocSize[NumHeapTypes] = { 0 };

//------------------------------------------------------------------------------
/**
    Debug function which validates the process heap and all local heaps. 
    Stops the program if something is wrong. 
*/
bool
Validate()
{
    bool res = true;
    IndexT i;
    for (i = 0; i < NumHeapTypes; i++)
    {
        if (0 != Heaps[i])
        {
            res &= (0 != HeapValidate(Heaps[i], 0, NULL));
        }
    }
    res &= Heap::ValidateAllHeaps();
    return res;
}

#endif
#if NEBULA3_MEMORY_ADVANCED_DEBUGGING
void DumpMemoryLeaks()
{
	// FIXME
}
#endif

} // namespace Memory
