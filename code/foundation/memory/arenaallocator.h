#pragma once
//------------------------------------------------------------------------------
/**
    @class Memory::ArenaAllocator
    
    Allocates memory in chunks.

    This allocator creates memory in user-specified (default 65535) byte chunks.
    Each time an object is requested, the current chunk is checked for storage,
    and if the object fits, an iterator is progressed with the size of the object,
    after which a pointer to the allocated space is returned. If the object does not
    fit, the allocator creates a new chunk, and retires the old, without
    considering the potential space lost in the process.

    This type of allocator allows for several objects of different types to be
    allocated linearly in memory, thus providing a cache-friendly access pattern
    for places where memory allocation is somewhat deterministic. 

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/config.h"
#include "util/array.h"

#define SMALL_CHUNK 0x100
#define MEDIUM_CHUNK 0x1000
#define BIG_CHUNK 0x10000
namespace Memory
{


template <int ChunkSize>
class ArenaAllocator
{
public:
    /// constructor
    ArenaAllocator();
    /// destructor
    ~ArenaAllocator();

    /// copy constructor
    ArenaAllocator(const ArenaAllocator& rhs);
    /// assignment operator
    void operator=(const ArenaAllocator& rhs);

    /// move constructor
    ArenaAllocator(ArenaAllocator&& rhs);
    /// move operator
    void operator=(ArenaAllocator&& rhs);
    
    /// allocate new object, and calls constructor, but beware because this allocator does not run the destructors
    template <typename T> T* Alloc();
    /// allocate new chunk of size
    void* Alloc(SizeT size);
    /// retires a chunk and creates a new one (might waste memory)
    void NewChunk();
    /// release all memory
    void Release();

private:
    byte* currentChunk;
    byte* iterator;
    Util::Array<byte*> retiredChunks;
};

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
inline
ArenaAllocator<ChunkSize>::ArenaAllocator() :
    currentChunk(nullptr),
    iterator(nullptr)
{
    // constructor
}

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
inline
ArenaAllocator<ChunkSize>::~ArenaAllocator()
{
    this->currentChunk = nullptr;
    this->iterator = nullptr;
    this->retiredChunks.Clear();
    //this->Release(); // perhaps call release when we actually want to dealloc
}

//------------------------------------------------------------------------------
/**
*/
template<int ChunkSize>
inline
ArenaAllocator<ChunkSize>::ArenaAllocator(ArenaAllocator&& rhs)
{
    this->retiredChunks = rhs.retiredChunks;
    this->currentChunk = rhs.currentChunk;
    this->iterator = rhs.iterator;

    rhs.retiredChunks.Clear();
    rhs.currentChunk = nullptr;
    rhs.iterator = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<int ChunkSize>
inline
ArenaAllocator<ChunkSize>::ArenaAllocator(const ArenaAllocator& rhs)
{
    // copy chunk
    this->retiredChunks = rhs.retiredChunks;
    this->currentChunk = rhs.currentChunk;
    this->iterator = rhs.iterator;
    /*
    IndexT i;
    for (i = 0; i < rhs.retiredChunks.Size(); i++)
    {
        byte* chunk = (byte*)Memory::Alloc(ObjectArrayHeap, ChunkSize);
        memcpy(chunk, rhs.retiredChunks[i], ChunkSize);
        this->retiredChunks.Append(chunk);
    }
    if (rhs.currentChunk)
    {
        this->currentChunk = (byte*)Memory::Alloc(ObjectArrayHeap, ChunkSize);
        ptrdiff_t size = rhs.iterator - rhs.currentChunk;
        memcpy(this->currentChunk, rhs.currentChunk, size);
        this->iterator = this->currentChunk + size;
    }
    else
    {
        this->currentChunk = nullptr;
        this->iterator = nullptr;
    }
    */
}

//------------------------------------------------------------------------------
/**
*/
template<int ChunkSize>
inline void
ArenaAllocator<ChunkSize>::operator=(ArenaAllocator&& rhs)
{
    this->retiredChunks = rhs.retiredChunks;
    this->currentChunk = rhs.currentChunk;
    this->iterator = rhs.iterator;

    rhs.retiredChunks.Clear();
    rhs.currentChunk = nullptr;
    rhs.iterator = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template<int ChunkSize>
inline void
ArenaAllocator<ChunkSize>::operator=(const ArenaAllocator& rhs)
{
    this->retiredChunks = rhs.retiredChunks;
    this->currentChunk = rhs.currentChunk;
    this->iterator = rhs.iterator;
    /*
    IndexT i;
    for (i = 0; i < rhs.retiredChunks.Size(); i++)
    {
        byte* chunk = (byte*)Memory::Alloc(ObjectArrayHeap, ChunkSize);
        memcpy(chunk, rhs.retiredChunks[i], ChunkSize);
        this->retiredChunks.Append(chunk);
    }
    if (rhs.currentChunk)
    {
        this->currentChunk = (byte*)Memory::Alloc(ObjectArrayHeap, ChunkSize);
        ptrdiff_t size = rhs.iterator - rhs.currentChunk;
        memcpy(this->currentChunk, rhs.currentChunk, size);
        this->iterator = this->currentChunk + size;
    }
    else
    {
        this->currentChunk = nullptr;
        this->iterator = nullptr;
    }
    */
}

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
inline void
ArenaAllocator<ChunkSize>::NewChunk()
{
    if (this->currentChunk != nullptr)
        this->retiredChunks.Append(this->currentChunk);
    this->currentChunk = (byte*)Memory::Alloc(ObjectArrayHeap, ChunkSize);
    this->iterator = this->currentChunk;
}

//------------------------------------------------------------------------------
/**
*/
template<int ChunkSize>
inline void ArenaAllocator<ChunkSize>::Release()
{
    IndexT i;
    for (i = 0; i < this->retiredChunks.Size(); i++)
    {
        Memory::Free(ObjectArrayHeap, this->retiredChunks[i]);
    }
    if (this->currentChunk)
        Memory::Free(ObjectArrayHeap, this->currentChunk);
    this->retiredChunks.Clear();
    this->currentChunk = nullptr;
    this->iterator = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
template<typename T>
inline T*
ArenaAllocator<ChunkSize>::Alloc()
{
    static_assert(sizeof(T) <= ChunkSize, "Size of type is bigger than the chunk size!");
    // pad up to next multiple of 16 to avoid alignment issues
    SizeT alignedSize = Math::align(sizeof(T), 16);
    if (this->iterator == nullptr)
    {
        this->NewChunk();
    }
    else
    {
        // we cast the pointer diff but it should be safe since it should never be above ChunkSize
        SizeT remainder = ChunkSize - SizeT(this->iterator - this->currentChunk);       
        if (remainder < alignedSize)
            this->NewChunk();
    }

    T* ret = new (this->iterator) T;
    this->iterator += alignedSize;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
inline void*
ArenaAllocator<ChunkSize>::Alloc(SizeT size)
{
    // pad to next alignment.
    size = Math::align(size, 16);
    n_assert(size <= ChunkSize);
    n_assert(size != 0);
    if (this->iterator == nullptr)
    {
        this->NewChunk();
    }
    else
    {
        PtrDiff remainder = this->currentChunk + ChunkSize - this->iterator;
        if (remainder < size)
            this->NewChunk();
    }
    void* ret = this->iterator;
    this->iterator += size;
    return ret;
}

} // namespace Memory
