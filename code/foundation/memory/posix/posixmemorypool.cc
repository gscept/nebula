//------------------------------------------------------------------------------
//  osxmemorypool.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memory/posix/posixmemorypool.h"

namespace Posix
{
    
//------------------------------------------------------------------------------
/**
*/
PosixMemoryPool::PosixMemoryPool()
{
    n_error("IMPLEMENT ME!\n");
}

//------------------------------------------------------------------------------
/**
*/
PosixMemoryPool::~PosixMemoryPool()
{
    n_error("IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
    NOTE: name must be a static string!
*/
void
PosixMemoryPool::Setup(Memory::HeapType heapType_, uint blockSize_, uint numBlocks_)
{
    n_error("IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
*/
void*
PosixMemoryPool::Alloc()
{
    n_error("IMPLEMENT ME!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
void
PosixMemoryPool::Free(void* ptr)
{
    n_error("IMPLEMENT ME!\n");
}
    
} // namespace OSX
