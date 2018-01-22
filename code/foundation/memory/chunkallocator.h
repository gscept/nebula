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

class ChunkAllocator
{
public:
	/// constructor
	ChunkAllocator(const SizeT size = 0xFFFF); // 65535 byte
	/// destructor
	~ChunkAllocator();
	
	/// allocate new object
	template <typename T> T* Alloc();
	/// retires a chunk and creates a new one (might waste memory)
	void NewChunk();
private:
	const SizeT chunkSize;
	byte* currentChunk;
	byte* iterator;
	Util::Array<byte*> retiredChunks;
};

//------------------------------------------------------------------------------
/**
*/
inline
ChunkAllocator::ChunkAllocator(const SizeT size) :
	chunkSize(size),
	iterator(nullptr)
{
	n_assert(size > 0);
	this->currentChunk = new byte[chunkSize];
	this->iterator = this->currentChunk;
}

//------------------------------------------------------------------------------
/**
*/
inline
ChunkAllocator::~ChunkAllocator()
{
	IndexT i;
	for (i = 0; i < this->retiredChunks.Size(); i++)
	{
		delete[] this->retiredChunks[i];
	}
	delete[] this->currentChunk;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ChunkAllocator::NewChunk()
{
	this->retiredChunks.Append(this->currentChunk);
	this->currentChunk = new byte[this->chunkSize];
	this->iterator = this->currentChunk;
}

//------------------------------------------------------------------------------
/**
*/
template<typename T>
inline T*
ChunkAllocator::Alloc()
{
	n_assert(sizeof(T) <= this->chunkSize);
	ptrdiff remainder = this->currentChunk + this->chunkSize - this->iterator;
	if (remainder < sizeof(T))
		this->NewChunk();

	T* ret = new (this->iterator) T();
	this->iterator += sizeof(T);
	return ret;
}

} // namespace Memory
