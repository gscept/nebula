#pragma once
//------------------------------------------------------------------------------
/**
    @file memory/win32/win32memory.h

    Win32 specific low level memory functions.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
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
    