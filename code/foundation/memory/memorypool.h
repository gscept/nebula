#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::MemoryPool
    
    A simple thread-safe memory pool. Memory pool items are 16-byte aligned.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__XBOX360__ || __WIN32__)
#include "memory/win360/win360memorypool.h"
namespace Memory
{
typedef Win360::Win360MemoryPool MemoryPool;
}
#elif __WII__
#include "memory/wii/wiimemorypool.h"
namespace Memory
{
typedef Wii::WiiMemoryPool MemoryPool;
}
#elif __PS3__
#include "memory/ps3/ps3memorypool.h"
namespace Memory
{
typedef PS3::PS3MemoryPool MemoryPool;
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
    