#pragma once
//------------------------------------------------------------------------------
/**
    @file memory.h
  
    Implements a memory related functions.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#include "core/types.h"

namespace Memory
{
//------------------------------------------------------------------------------
/**
*/
__forceinline unsigned int
align(unsigned int alignant, unsigned int alignment)
{
    return (alignant + alignment - 1) & ~(alignment - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline unsigned int
align(int alignant, int alignment)
{
    return ((unsigned int)alignant + (unsigned int)alignment - 1) & ~((unsigned int)alignment - 1);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline size_t
align(size_t alignant, size_t alignment)
{
    return (alignant + alignment - 1) & ~(alignment - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uintptr_t
alignptr(uintptr_t alignant, uintptr_t alignment)
{
    return (alignant + alignment - 1) & ~(alignment - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline unsigned int
align_down(unsigned int alignant, unsigned int alignment)
{
    return (alignant / alignment * alignment);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline size_t
align_down(size_t alignant, size_t alignment)
{
    return (alignant / alignment * alignment);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uintptr_t
align_downptr(uintptr_t alignant, uintptr_t alignment)
{
    return (alignant / alignment * alignment);
}
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint64_t
operator"" _KB(const unsigned long long val)
{
    return val * 1024;
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint64_t
operator"" _MB(const unsigned long long val)
{
    return val * 1024 * 1024;
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint64_t
operator"" _GB(const unsigned long long val)
{
    return val * 1024 * 1024 * 1024;
}

#if (__WIN32__)
#include "memory/win32/win32memory.h"
#elif ( __OSX__ || __APPLE__ || __linux__ )
#include "memory/posix/posixmemory.h"
#else
#error "UNKNOWN PLATFORM"
#endif

struct ThreadLocalMiniHeap
{
    ThreadLocalMiniHeap();
    ~ThreadLocalMiniHeap();
    
    void Realloc(size_t numBytes);

    char* heap;
    size_t iterator;
    size_t capacity;
};

extern thread_local ThreadLocalMiniHeap N_ThreadLocalMiniHeap;

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
TYPE* 
ArrayAlloc(size_t size)
{
    TYPE* buffer = (TYPE*)Memory::Alloc(Memory::ObjectArrayHeap, size * sizeof(TYPE));
    if constexpr (!std::is_trivially_constructible<TYPE>::value)
    {
        for (size_t i = 0; i < size; ++i)
        {
            ::new( &buffer[i] ) TYPE;
        }
    }
    return buffer;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
TYPE*
ArrayAllocStack(size_t size)
{
    if (size == 0) return nullptr;
    TYPE* buffer = (TYPE*)(N_ThreadLocalMiniHeap.heap + N_ThreadLocalMiniHeap.iterator);
    const size_t bytes = size * sizeof(TYPE) + (sizeof(void*) - 1);
    buffer = (TYPE*)Memory::alignptr((uintptr_t)buffer, sizeof(void*));
        
    // Bounds check. This can never be disabled, as we might go OOB, which can
    // cause buffer overflows and other security issues.
    if (N_ThreadLocalMiniHeap.iterator + bytes >= N_ThreadLocalMiniHeap.capacity)
    {
        // If you run into this error, you're using too much stack memory.
        // Consider using a separate allocator!
        n_error("ArrayAllocStack is out of bounds!");
        return nullptr;
    }
    
    if constexpr (!std::is_trivially_constructible<TYPE>::value)
    {
        for (size_t i = 0; i < size; ++i)
        {
            ::new(&buffer[i]) TYPE;
        }
    }
    N_ThreadLocalMiniHeap.iterator += bytes;
    return buffer;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void 
ArrayFree(size_t size, TYPE* buffer)
{
    if constexpr (!std::is_trivially_destructible<TYPE>::value)
    {
        for (size_t i = 0; i < size; ++i)
        {
            buffer[i].~TYPE();
        }
    }
    Memory::Free(Memory::ObjectArrayHeap, (void*)buffer);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void
ArrayFreeStack(size_t size, TYPE* buffer)
{
    if (size == 0)
        return;
    const size_t bytes = size * sizeof(TYPE) + (sizeof(void*) - 1);
    char* topPtr = (N_ThreadLocalMiniHeap.heap + N_ThreadLocalMiniHeap.iterator - bytes);
    topPtr = (char*)Memory::alignptr((uintptr_t)topPtr, sizeof(void*));
    n_assert(buffer == (TYPE*)topPtr);
    if constexpr (!std::is_trivially_destructible<TYPE>::value)
    {
        for (size_t i = 0; i < size; ++i)
        {
            buffer[i].~TYPE();
        }
    }
    N_ThreadLocalMiniHeap.iterator -= bytes;
}
