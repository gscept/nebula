#pragma once
//------------------------------------------------------------------------------
/**
	This allocator creates memory in user-specified (default 65535) byte chunks.
	Each time an object is requested, the current chunk is checked for storage,
	and if the object fits, an iterator is progressed with the size of the object,
	after which a pointer to the allocated space is returned. If the object does not
	fit, the allocator creates a new chunk, and retires the old, without
	considering the potential space lost in the process.

	This type of allocator allows for several objects of different types to be
	allocated linearly in memory, thus providing a cache-friendly access pattern
	for places where memory allocation is somewhat deterministic. 

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/config.h"
#include "util/array.h"
namespace Memory
{

template <int ChunkSize>
class ChunkAllocator
{
public:
	/// constructor
	ChunkAllocator();
	/// destructor
	~ChunkAllocator();
	
	/// allocate new object
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
ChunkAllocator<ChunkSize>::ChunkAllocator() :
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
ChunkAllocator<ChunkSize>::~ChunkAllocator()
{
	this->Release();
}

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
inline void
ChunkAllocator<ChunkSize>::NewChunk()
{
	if (this->currentChunk != nullptr)
		this->retiredChunks.Append(this->currentChunk);
	this->currentChunk = new byte[ChunkSize];
	this->iterator = this->currentChunk;
}

//------------------------------------------------------------------------------
/**
*/
template<int ChunkSize>
inline void ChunkAllocator<ChunkSize>::Release()
{
	IndexT i;
	for (i = 0; i < this->retiredChunks.Size(); i++)
	{
		delete[] this->retiredChunks[i];
	}
	if (this->currentChunk) delete[] this->currentChunk;
	this->retiredChunks.Clear();
	this->currentChunk = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
template<typename T>
inline T*
ChunkAllocator<ChunkSize>::Alloc()
{
	n_assert(sizeof(T) <= ChunkSize);
	if (this->iterator == nullptr)
	{
		this->NewChunk();
	}
	else
	{
		PtrDiff remainder = this->currentChunk + ChunkSize - this->iterator;
		if (remainder < sizeof(T))
			this->NewChunk();
	}

	T* ret = new (this->iterator) T();
	this->iterator += sizeof(T);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
template <int ChunkSize>
inline void*
ChunkAllocator<ChunkSize>::Alloc(SizeT size)
{
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
