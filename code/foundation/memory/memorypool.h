#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::MemoryPool
    
    A simple thread-safe memory pool. Memory pool items are 16-byte aligned.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__)
#include "memory/win360/win360memorypool.h"
namespace Memory
{
typedef Win360::Win360MemoryPool MemoryPool;
}
#elif ( __OSX__ || __APPLE__ || __linux__ )
#include "memory/posix/posixmemorypool.h"
namespace Memory
{
typedef Posix::PosixMemoryPool MemoryPool;
}
#else
#error "IMPLEMENT ME!"
#endif
