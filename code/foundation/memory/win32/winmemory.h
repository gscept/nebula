#pragma once
//------------------------------------------------------------------------------
/**
    @file memory/win/winmemory.h

    Windows specific low level memory functions.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#include "core/debug.h"

namespace Memory
{
//------------------------------------------------------------------------------
/**
    Copy a chunk of memory (note the argument order is different from memcpy()!!!)
*/
__forceinline void
Copy(const void* from, void* to, size_t numBytes)
{
    if (numBytes > 0)
    {
        n_assert(0 != from);
        n_assert(0 != to);
        n_assert(from != to);
        CopyMemory(to, from, numBytes);
    }
}

//------------------------------------------------------------------------------
/**
    Copy a chunk of memory (note the argument order is different from memcpy()!!!)
*/
template <typename T>
__forceinline void
CopyElements(const T* from, T* to, size_t numElements)
{
    if (numElements > 0)
    {
        n_assert(0 != from);
        n_assert(0 != to);
        n_assert(from != to);
        CopyMemory(to, from, numElements * sizeof(T));
    }
}

//------------------------------------------------------------------------------
/**
    Move a chunk of memory, can handle overlapping regions
*/
__forceinline void
Move(const void* from, void* to, size_t numBytes)
{
    if (numBytes > 0)
    {
        n_assert(0 != from);
        n_assert(0 != to);
        n_assert(from != to);
        MoveMemory(to, from, numBytes);
    }
}

//------------------------------------------------------------------------------
/**
    Copy data from a system memory buffer to graphics resource memory. Some
    platforms may need special handling of this case.
*/
__forceinline void
CopyToGraphicsMemory(const void* from, void* to, size_t numBytes)
{
    // no special handling on the Win32 platform
    Memory::Copy(from, to, numBytes);
}

//------------------------------------------------------------------------------
/**
    Overwrite a chunk of memory with 0's.
*/
__forceinline void
Clear(void* ptr, size_t numBytes)
{
    ZeroMemory(ptr, numBytes);
}

//------------------------------------------------------------------------------
/**
    Fill memory with a specific byte.
*/
__forceinline void
Fill(void* ptr, size_t numBytes, unsigned char value)
{
    FillMemory(ptr, numBytes, value);
}

} // namespace Memory
//------------------------------------------------------------------------------
    