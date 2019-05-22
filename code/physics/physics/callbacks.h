#pragma once
//------------------------------------------------------------------------------
/**
	callback interfaces for PhysX	
	
	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "memory/memory.h"

//------------------------------------------------------------------------------
namespace Physics
{

//------------------------------------------------------------------------------
class Allocator: public physx::PxAllocatorCallback
{
public:
    // FIXME check for memleaks? 
    ~Allocator() {  }
    /// physx callback for allocating memory, should be at least 16 byte aligned
    void* allocate(size_t size, const char* typeName, const char* filename, int line);
    /// 
    void deallocate(void* ptr);
};


//------------------------------------------------------------------------------

class ErrorCallback : public physx::PxErrorCallback
{
public:
    /// error callback channeling px messages to nebula log system
    void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line);
};

//------------------------------------------------------------------------------
/**
*/
inline void*
Allocator::allocate(size_t size, const char * typeName, const char* filename, int line)
{
    void* buffer = Memory::Alloc(Memory::HeapType::PhysicsHeap, size);
    n_assert_fmt(buffer, "Allocation of %s failed: %s, line: %d", typeName, filename, line);
    return buffer;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Allocator::deallocate(void * ptr)
{
    Memory::Free(Memory::HeapType::PhysicsHeap, ptr);
}

}



