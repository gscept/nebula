//------------------------------------------------------------------------------
//  posixmemoryconfig.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "memory/posix/posixmemoryconfig.h"
#include "core/sysfunc.h"

namespace Memory
{
void* volatile Heaps[NumHeapTypes] = { NULL };

//------------------------------------------------------------------------------
/**
*/
void
SetupHeaps()
{
    const size_t megaByte = 1024 * 1024;
    unsigned int i;
    for (i = 0; i < NumHeapTypes; i++)
    {
        n_assert(0 == Heaps[i]);
        size_t initialSize = 0;
        size_t maxSize = 0;
        bool useLowFragHeap = false;
        switch (i)
        {
            case DefaultHeap:
            case ObjectHeap:
            case ObjectArrayHeap:
                // these heaps start at 8 megabyte and may grow
                initialSize = 4 * megaByte;
                useLowFragHeap = true;
                break;

            case ResourceHeap:
            case PhysicsHeap:
            case AppHeap:
            case NetworkHeap:
            case ScriptingHeap:
                // the large block and resource heaps start at 32 megabyte and may grow(?)
                initialSize = 8 * megaByte;
                break;

            case ScratchHeap:
                // the scratch heap is set to 8 megabyte and may not grow
                initialSize = 8 * megaByte;
                break;

            case StringDataHeap:
                // the string heap starts at 2 MB and may grow
                initialSize = 2 * megaByte;
                useLowFragHeap = true;
                break;

            case StreamDataHeap:
                // the stream data heap starts at 16 MB and may grow
                initialSize = 16 * megaByte;
                break;

            default:
                Core::SysFunc::Error("Invalid heap type in Memory::SetupHeaps() (posixmemoryconfig.cc)!");
                break;
        }
        if (0 != initialSize)
        {
            Heaps[i] = NULL; // XXX: HeapCreate(HEAP_GENERATE_EXCEPTIONS, initialSize, maxSize);
            if (useLowFragHeap)
            {
                #if __WIN32__
                // enable the Win32 LowFragmentationHeap
                ULONG heapFragValue = 2;
                HeapSetInformation(Heaps[i], HeapCompatibilityInformation, &heapFragValue, sizeof(heapFragValue));
                #endif
            }
        }
        else
        {
            Heaps[i] = 0;
        }
    }
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
        case ScriptingHeap:             return "Scripting Heap";
        default:
            Core::SysFunc::Error("Invalid HeapType arg in Memory::GetHeapTypeName()! (win32memoryconfig.cc)");
            return 0;
    }
}

} // namespace Memory
