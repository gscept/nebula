#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::Heap
  
    Implements a private heap.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__)
#include "memory/win360/win360heap.h"
namespace Memory
{
typedef Win360::Win360Heap Heap;
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
