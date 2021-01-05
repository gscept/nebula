#pragma once
//------------------------------------------------------------------------------
/**
    @file memory/win32/win32memoryconfig.h
    
    Central config file for memory setup on the Win32 platform.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#include "core/debug.h"

namespace Memory
{

//------------------------------------------------------------------------------
/**
    Heap types are defined here. The main purpose for the different heap
    types is to decrease memory fragmentation and to improve cache
    usage by grouping similar data together. Platform ports may define
    platform-specific heap types, as long as only platform specific
    code uses those new heap types.
*/
enum HeapType
{
    DefaultHeap = 0,            // for stuff that doesn't fit into any category
    ObjectHeap,                 // heap for global new allocator
    ObjectArrayHeap,            // heap for global new[] allocator
    ResourceHeap,               // heap for resource data (like animation buffers)
    ScratchHeap,                // for short-lived scratch memory (encode/decode buffers, etc...)
    StringDataHeap,             // special heap for string data
    StreamDataHeap,             // special heap for stream data like memory streams, zip file streams, etc...
    PhysicsHeap,                // physics engine allocations go here
    AppHeap,                    // for general Application layer stuff
    NetworkHeap,                // for network layer
    
    NumHeapTypes,
    InvalidHeapType,
};

//------------------------------------------------------------------------------
/**
    Heap pointers are defined here. Call ValidateHeap() to check whether
    a heap already has been setup, and to setup the heap if not.
*/
extern HANDLE volatile Heaps[NumHeapTypes];

//------------------------------------------------------------------------------
/**
    This method is called by SysFunc::Setup() to setup the different heap
    types. This method can be tuned to define the start size of the 
    heaps and whether the heap may grow or not (non-growing heaps may
    be especially useful on console platforms without memory paging).
*/
extern void SetupHeaps();

//------------------------------------------------------------------------------
/**
    Returns a human readable name for a heap type.
*/
extern const char* GetHeapTypeName(HeapType heapType);

//------------------------------------------------------------------------------
/**
    Global PoolArrayAllocator objects, these are all setup in a central
    place in the Memory::SetupHeaps() function!
*/
#if NEBULA_OBJECTS_USE_MEMORYPOOL
class PoolArrayAllocator;
extern PoolArrayAllocator* ObjectPoolAllocator;  // Rtti::AllocInstanceMemory() and new operators alloc from here
#endif    

//------------------------------------------------------------------------------
/**
    Helper function for Heap16 functions: aligns pointer to 16 byte and 
    writes padding mask to byte before returned pointer.
*/
__forceinline unsigned char*
__HeapAlignPointerAndWritePadding16(unsigned char* ptr)
{
    unsigned char paddingMask = size_t(ptr) & 15;
    ptr = (unsigned char*)(size_t(ptr + 16) & ~15);
    ptr[-1] = paddingMask;
    return ptr;
}

//------------------------------------------------------------------------------
/**
    Helper function for Heap16 functions: "un-aligns" pointer through
    the padding mask stored in the byte before the pointer.
*/
__forceinline unsigned char*
__HeapUnalignPointer16(unsigned char* ptr)
{
    return (unsigned char*)(size_t(ptr - 16) | ptr[-1]);
}

//------------------------------------------------------------------------------
/**
    HeapAlloc replacement which always returns 16-byte aligned addresses.

    NOTE: only works for 32 bit pointers!
*/
__forceinline LPVOID 
__HeapAlloc16(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
    unsigned char* ptr = (unsigned char*) ::HeapAlloc(hHeap, dwFlags, dwBytes + 16);
    ptr = __HeapAlignPointerAndWritePadding16(ptr);
    return (LPVOID) ptr;
}

//------------------------------------------------------------------------------
/**
    HeapReAlloc replacement for 16-byte alignment.

    NOTE: only works for 32 bit pointers!
*/
__forceinline LPVOID
__HeapReAlloc16(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes)
{
    // restore unaligned pointer
    unsigned char* ptr = (unsigned char*) lpMem;
    unsigned char* rawPtr = __HeapUnalignPointer16(ptr); 

    // perform re-alloc, NOTE: if re-allocation can't happen in-place,
    // we need to handle the allocation ourselves, in order not to destroy 
    // the original data because of different alignment!!!
    ptr = (unsigned char*) ::HeapReAlloc(hHeap, (dwFlags | HEAP_REALLOC_IN_PLACE_ONLY), rawPtr, dwBytes + 16);
    if (0 == ptr)
    {                   
        SIZE_T rawSize = ::HeapSize(hHeap, dwFlags, rawPtr);        
        // re-allocate manually because padding may be different!
        ptr = (unsigned char*) ::HeapAlloc(hHeap, dwFlags, dwBytes + 16);
        ptr = __HeapAlignPointerAndWritePadding16(ptr);
        SIZE_T copySize = dwBytes <= (rawSize - 16) ? dwBytes : (rawSize - 16);
        ::CopyMemory(ptr, lpMem, copySize);    
        // release old mem block
        ::HeapFree(hHeap, dwFlags, rawPtr);
    }
    else
    {
        // was re-allocated in place
        ptr = __HeapAlignPointerAndWritePadding16(ptr);
    }
    return (LPVOID) ptr;
}

//------------------------------------------------------------------------------
/**
    HeapFree replacement which always returns 16-byte aligned addresses.

    NOTE: only works for 32 bit pointers!
*/
__forceinline BOOL
__HeapFree16(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    unsigned char* ptr = (unsigned char*) lpMem;
    ptr = __HeapUnalignPointer16(ptr);
    return ::HeapFree(hHeap, dwFlags, ptr);
}

//------------------------------------------------------------------------------
/**
    HeapSize replacement function.
*/
__forceinline SIZE_T
__HeapSize16(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem)
{
    unsigned char* ptr = (unsigned char*) lpMem;
    ptr = __HeapUnalignPointer16(ptr);
    return ::HeapSize(hHeap, dwFlags, ptr);
}    

} // namespace Memory    
//------------------------------------------------------------------------------
