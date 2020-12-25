//------------------------------------------------------------------------------
//  win32memoryconfig.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "memory/win32/win32memoryconfig.h"
#include "core/sysfunc.h"

#if NEBULA_OBJECTS_USE_MEMORYPOOL
#include "memory/poolarrayallocator.h"
#endif

namespace Memory
{
HANDLE volatile Heaps[NumHeapTypes] = { NULL };

#if NEBULA_OBJECTS_USE_MEMORYPOOL
PoolArrayAllocator* ObjectPoolAllocator = 0;
#endif

//------------------------------------------------------------------------------
/**
    This method is called once at application startup from 
    Core::SysFunc::Setup() to setup the various Nebula heaps.
*/
void
SetupHeaps()
{
    // setup global heaps
    const SIZE_T megaByte = 1024 * 1024;
    const SIZE_T kiloByte = 1024;
    unsigned int i;
    for (i = 0; i < NumHeapTypes; i++)
    {
        n_assert(0 == Heaps[i]);
        SIZE_T initialSize = 0;
        SIZE_T maxSize = 0;
        bool useLowFragHeap = false;
        switch (i)
        {
            case DefaultHeap:
            case ObjectHeap:
                initialSize = 4 * megaByte;
                useLowFragHeap = true;
                break;

            case ObjectArrayHeap:
                initialSize = 4 * megaByte;
                break;

            case ResourceHeap:
            case PhysicsHeap:
            case AppHeap:
            case NetworkHeap:
                initialSize = 8 * megaByte;
                break;

            case ScratchHeap:
                initialSize = 8 * megaByte;
                break;

            case StringDataHeap:
                initialSize = 2 * megaByte;
                useLowFragHeap = true;
                break;

            case StreamDataHeap:
                initialSize = 16 * megaByte;
                break;

            default:
                Core::SysFunc::Error("Invalid heap type in Memory::SetupHeaps() (win360memoryconfig.cc)!");
                break;
        }
        if (0 != initialSize)
        {
            Heaps[i] = HeapCreate(0, initialSize, maxSize);
            if (useLowFragHeap)
            {
                // enable the Win32 LowFragmentationHeap
                ULONG heapFragValue = 2;
                HeapSetInformation(Heaps[i], HeapCompatibilityInformation, &heapFragValue, sizeof(heapFragValue));
            }
        }
        else
        {
            Heaps[i] = 0;
        }
    }

    #if NEBULA_OBJECTS_USE_MEMORYPOOL        
    // setup the RefCounted pool allocator
    // HMM THESE NUMBERS ARE SO HIGH BECAUSE OF GODSEND...
    #if NEBULA_DEBUG
    uint objectPoolSizes[PoolArrayAllocator::NumPools] = {
        128 * kiloByte,     // 28 byte blocks
        4196 * kiloByte,    // 60 byte blocks
        512 * kiloByte,     // 92 byte blocks
        1024 * kiloByte,    // 124 byte blocks
        128 * kiloByte,     // 156 byte blocks
        128 * kiloByte,     // 188 byte blocks
        128 * kiloByte,     // 220 byte blocks
        2048 * kiloByte     // 252 byte blocks
    };
    #else
    uint objectPoolSizes[PoolArrayAllocator::NumPools] = {
        2048 * kiloByte,    // 28 byte blocks
        2048 * kiloByte,    // 60 byte blocks
        1024 * kiloByte,    // 92 byte blocks
        1024 * kiloByte,    // 124 byte blocks
        128 * kiloByte,     // 156 byte blocks
        128 * kiloByte,     // 188 byte blocks
        2048 * kiloByte,    // 220 byte blocks
        2048 * kiloByte     // 252 byte blocks
    };
    #endif   
    ObjectPoolAllocator = n_new(PoolArrayAllocator);
    ObjectPoolAllocator->Setup("ObjectPoolAllocator", ObjectHeap, objectPoolSizes);
    #endif
}

//------------------------------------------------------------------------------
/**
*/
const char*
GetHeapTypeName(HeapType heapType)
{
    switch (heapType)
    {
        case DefaultHeap:               return "Default Heap";
        case ObjectHeap:                return "Object Heap";
        case ObjectArrayHeap:           return "Object Array Heap";
        case ResourceHeap:              return "Resource Heap";
        case ScratchHeap:               return "Scratch Heap";
        case StringDataHeap:            return "String Data Heap";
        case StreamDataHeap:            return "Stream Data Heap";
        case PhysicsHeap:               return "Physics Heap";
        case AppHeap:                   return "App Heap";
        case NetworkHeap:               return "Network Heap";
        default:
            Core::SysFunc::Error("Invalid HeapType arg in Memory::GetHeapTypeName()! (win360memoryconfig.cc)");
            return 0;
    }
}

} // namespace Win32
