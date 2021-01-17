#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::Heap
  
    Implements a private heap.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__)
#include "memory/win32/win32heap.h"
namespace Memory
{
typedef Win32::Win32Heap Heap;
}
#elif ( __OSX__ || __APPLE__ || __linux__ )
#include "memory/posix/posixheap.h"
namespace Memory
{
typedef Posix::PosixHeap Heap;
}
#else
#error "IMPLEMENT ME!"
#endif
