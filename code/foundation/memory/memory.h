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

#if (__WIN32__)
#include "memory/win32/win32memory.h"
#elif ( __OSX__ || __APPLE__ || __linux__ )
#include "memory/posix/posixmemory.h"
#else
#error "UNKNOWN PLATFORM"
#endif

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
