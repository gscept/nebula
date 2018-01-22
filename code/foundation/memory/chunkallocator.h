#pragma once
//------------------------------------------------------------------------------
/**
	The chunk allocator allocates a chunk of memory and performs in-place new on it,
	up until the chunk runs out of memory. When that happens, the chunk is retired,
	and another one is created. It ensures that all objects created from the 
	allocator appear local in memory, however only if it can fit within a chunk. 

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
