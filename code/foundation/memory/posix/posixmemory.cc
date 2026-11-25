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
    Sized global delete operator. The size is not used, the heap knows the block,
    but the overload has to exist for the sized deallocation to be used.
*/
void
operator delete(void* p, size_t size) noexcept
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

//------------------------------------------------------------------------------
/**
    Sized global delete[] operator. See the sized delete operator above.
*/
void
operator delete[](void* p, size_t size) noexcept
{
    Memory::Free(Memory::ObjectArrayHeap, p);
}

//------------------------------------------------------------------------------
/**
    Aligned and nothrow global delete operators. Every new operator above is
    replaced, so every delete operator has to be too. Otherwise memory from
    Memory::Alloc is released by the C++ runtime's own delete.
*/
void
operator delete(void* p, std::align_val_t) noexcept
{
    Memory::Free(Memory::ObjectHeap, p);
}
void
operator delete(void* p, size_t, std::align_val_t) noexcept
{
    Memory::Free(Memory::ObjectHeap, p);
}
void
operator delete(void* p, const std::nothrow_t&) noexcept
{
    Memory::Free(Memory::ObjectHeap, p);
}
void
operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept
{
    Memory::Free(Memory::ObjectHeap, p);
}

//------------------------------------------------------------------------------
/**
    Aligned and nothrow global delete[] operators, see above.
*/
void
operator delete[](void* p, std::align_val_t) noexcept
{
    Memory::Free(Memory::ObjectArrayHeap, p);
}
void
operator delete[](void* p, size_t, std::align_val_t) noexcept
{
    Memory::Free(Memory::ObjectArrayHeap, p);
}
void
operator delete[](void* p, const std::nothrow_t&) noexcept
{
    Memory::Free(Memory::ObjectArrayHeap, p);
}
void
operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept
{
    Memory::Free(Memory::ObjectArrayHeap, p);
}
