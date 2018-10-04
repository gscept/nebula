//------------------------------------------------------------------------------
//  osxmemorypool.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "memory/osx/osxmemorypool.h"

namespace OSX
{
    
//------------------------------------------------------------------------------
/**
*/
OSXMemoryPool::OSXMemoryPool()
{
    n_error("IMPLEMENT ME!\n");
}

//------------------------------------------------------------------------------
/**
*/
OSXMemoryPool::~OSXMemoryPool()
{
    n_error("IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
    NOTE: name must be a static string!
*/
void
OSXMemoryPool::Setup(Memory::HeapType heapType_, uint blockSize_, uint numBlocks_)
{
    n_error("IMPLEMENT ME!\n");
}
    
//------------------------------------------------------------------------------
/**
*/
void*
OSXMemoryPool::Alloc()
{
    n_error("IMPLEMENT ME!\n");
    return 0;
}
    
//------------------------------------------------------------------------------
/**
*/
void
OSXMemoryPool::Free(void* ptr)
{
    n_error("IMPLEMENT ME!\n");
}
    
} // namespace OSX
