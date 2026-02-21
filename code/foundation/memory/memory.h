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
    TYPE* buffer = (TYPE*)(N_ThreadLocalMiniHeap.heap + N_ThreadLocalMiniHeap.iterator);
    const size_t bytes = size * sizeof(TYPE);
        
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
    char* topPtr = (N_ThreadLocalMiniHeap.heap + N_ThreadLocalMiniHeap.iterator - size * sizeof(TYPE));
    n_assert(buffer == (TYPE*)topPtr);
    if constexpr (!std::is_trivially_destructible<TYPE>::value)
    {
        for (size_t i = 0; i < size; ++i)
        {
            buffer[i].~TYPE();
        }
    }
    N_ThreadLocalMiniHeap.iterator -= size * sizeof(TYPE);
}
