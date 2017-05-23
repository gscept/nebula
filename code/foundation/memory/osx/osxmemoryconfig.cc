//------------------------------------------------------------------------------
//  osxmemoryconfig.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memory/osx/osxmemoryconfig.h"
#include "core/sysfunc.h"

namespace Memory
{
malloc_zone_t* Heaps[NumHeapTypes];

//------------------------------------------------------------------------------
/**
    Setup the global heaps.
*/
void
SetupHeaps()
{
    const size_t megaByte = 1024 * 1024;
    unsigned int i;
    for (i = 0; i < NumHeapTypes; i++)
    {
        size_t startSize = 0;
        switch (i)
        {
            case DefaultHeap:       startSize = 2 * megaByte; break;
            case ObjectHeap:        startSize = 32 * megaByte; break;
            case ObjectArrayHeap:   startSize = 64 * megaByte; break;
            case ResourceHeap:      startSize = 8 * megaByte; break;
            case ScratchHeap:       startSize = 4 * megaByte; break;
            case StringDataHeap:    startSize = 1 * megaByte; break;
            case StreamDataHeap:    startSize = 32 * megaByte; break;
            case PhysicsHeap:       startSize = 1 * megaByte; break;
            case AppHeap:           startSize = 1 * megaByte; break;
                
            default:
                Core::SysFunc::Error("Memory::SetupHeaps(): invalid heap type!\n");
                break;
        }
        
        Heaps[i] = malloc_create_zone(startSize, 0);
        n_assert(0 != Heaps[i]);
    }
}
    
//------------------------------------------------------------------------------
/**
*/
const char*
GetHeapTypeName(HeapType heapType)
{
    switch(heapType)
    {
        case DefaultHeap:       return "Default Heap";
        case ObjectHeap:        return "Object Heap";
        case ObjectArrayHeap:   return "Object Array Heap";
        case ResourceHeap:              return "Resource Heap";
        case ScratchHeap:               return "Scratch Heap";
        case StringDataHeap:            return "String Data Heap";
        case StreamDataHeap:            return "Stream Data Heap";
        case PhysicsHeap:               return "Physics Heap";
        case AppHeap:                   return "App Heap";
        default:
            Core::SysFunc::Error("Invalid HeapType arg in Memory::GetHeapTypeName()!");
            return 0;
    }
}
    
} // namespace OSX