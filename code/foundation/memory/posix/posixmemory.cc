//------------------------------------------------------------------------------
//  win360memory.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/types.h"
#include "memory/heap.h"

namespace Memory
{
void* volatile PosixProcessHeap = 0;
#if NEBULA_MEMORY_STATS
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
#if NEBULA_MEMORY_ADVANCED_DEBUGGING
void DumpMemoryLeaks()
{
    // FIXME
}
#endif

} // namespace Memory

//------------------------------------------------------------------------------
/**
    Replacement global new operators.
*/
void*
operator new(size_t size) noexcept(false)
{
    return Memory::Alloc(Memory::ObjectHeap, size);
}
void* 
operator new(size_t size, std::align_val_t al) noexcept(false)
{
    return Memory::Alloc(Memory::ObjectHeap, size, (size_t)al);
}
void* operator new (size_t size, const std::nothrow_t& tag) noexcept
{
    return Memory::Alloc(Memory::ObjectHeap, size);
}
void* operator new (size_t size, std::align_val_t al, const std::nothrow_t& tag) noexcept
{
    return Memory::Alloc(Memory::ObjectHeap, size, (size_t)al);
}
//------------------------------------------------------------------------------
/**
    Replacement global new[] operators.
*/
void*
operator new[](size_t size) noexcept(false)
{
    return Memory::Alloc(Memory::ObjectArrayHeap, size);
}
void*
operator new[](size_t size, std::align_val_t al) noexcept(false)
{
    return Memory::Alloc(Memory::ObjectArrayHeap, size, (size_t)al);
}
void*
operator new[](size_t size, const std::nothrow_t& tag) noexcept
{
    return Memory::Alloc(Memory::ObjectArrayHeap, size);
}
void*
operator new[](size_t size, std::align_val_t al, const std::nothrow_t& tag) noexcept
{
    return Memory::Alloc(Memory::ObjectArrayHeap, size, (size_t)al);
}
//------------------------------------------------------------------------------
/**
    Replacement global delete operator.
*/
void
operator delete(void* p) noexcept
{
    Memory::Free(Memory::ObjectHeap, p);
}

//------------------------------------------------------------------------------
/**
    Replacement global delete[] operator.
*/
void
operator delete[](void* p) noexcept
{
    Memory::Free(Memory::ObjectArrayHeap, p);
}