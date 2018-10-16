#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Blob
    
    The Util::Blob class encapsulates a chunk of raw memory into 
    a C++ object which can be copied, compared and hashed.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "memory/heap.h"
#include "memory/poolarrayallocator.h"

//------------------------------------------------------------------------------
namespace Util
{
class Blob
{
public:
	/// static Setup method, called by SysFunc::Setup()
	static void Setup();
	/// static Shutdown method called by SysFunc::Exit
	static void Shutdown();
	/// override new operator
	void* operator new(size_t s);
	/// override delete operator
	void operator delete(void* ptr);

	/// default constructor
	Blob();
	/// nullptr constructor
	Blob(nullptr_t t);
	/// constructor
	Blob(const void* ptr, SizeT size);
	/// reserve N bytes
	Blob(SizeT size);
	/// copy constructor
	Blob(const Blob& rhs);
    /// move constructor
    Blob(Blob&& rhs);
	/// destructor
	~Blob();
	/// assignment operator
	void operator=(const Blob& rhs);
    /// move assignment operator
    void operator=(Blob&& rhs);

	/// equality operator
	bool operator==(const Blob& rhs) const;
	/// inequality operator
	bool operator!=(const Blob& rhs) const;
	/// greater operator
	bool operator>(const Blob& rhs) const;
	/// less operator
	bool operator<(const Blob& rhs) const;
	/// greater-equal operator
	bool operator>=(const Blob& rhs) const;
	/// less-eqial operator
	bool operator<=(const Blob& rhs) const;

	/// return true if the blob contains data
	bool IsValid() const;
	/// reserve N bytes
	void Reserve(SizeT size);
	/// trim the size member (without re-allocating!)
	void Trim(SizeT size);
	/// set blob contents
	void Set(const void* ptr, SizeT size);
    /// set from base64 enconded
    void SetFromBase64(const void* ptr, SizeT size);

	/// set chunk contents. Will allocate more memory if necessary.
	void SetChunk(const void* from, SizeT size, SizeT internalOffset);
	/// get blob ptr
	void* GetPtr() const;
	/// get blob size
	SizeT Size() const;
	/// get a hash code (compatible with Util::HashTable)
	IndexT HashCode() const;
    /// get as base64 encoded
    Util::Blob GetBase64() const;

private:
	/// delete content
	void Delete();
	/// allocate internal buffer
	void Allocate(SizeT size);
	/// copy content
	void Copy(const void* ptr, SizeT size);
	/// Increases allocated size without deleting existing data (reallocate and memcopy).
	void GrowTo(SizeT size);
	/// do a binary comparison between this and other blob
	int BinaryCompare(const Blob& rhs) const;

	static Memory::Heap* DataHeap;

	void* ptr;
	SizeT size;
	SizeT allocSize;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline void*
Blob::operator new(size_t size)
{
#if NEBULA_DEBUG
	n_assert(size == sizeof(Blob));
#endif

#if NEBULA_OBJECTS_USE_MEMORYPOOL
	return Memory::ObjectPoolAllocator->Alloc(size);
#else
	return Memory::Alloc(Memory::ObjectHeap, size);
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
Blob::operator delete(void* ptr)
{
#if NEBULA_OBJECTS_USE_MEMORYPOOL
	return Memory::ObjectPoolAllocator->Free(ptr, sizeof(Blob));
#else
	return Memory::Free(Memory::ObjectHeap, ptr);
#endif
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Setup()
{
	n_assert(0 == DataHeap);
	DataHeap = n_new(Memory::Heap("Util.Blob.DataHeap"));
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Shutdown()
{
	n_assert(0 != DataHeap);
	n_delete(DataHeap);
	DataHeap = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
Blob::Blob() :
	ptr(0),
	size(0),
	allocSize(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Blob::Blob(nullptr_t t) :
	ptr(0),
	size(0),
	allocSize(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
Blob::Blob(const void* fromPtr, SizeT fromSize) :
	ptr(0),
	size(0),
	allocSize(0)
{
	this->Copy(fromPtr, fromSize);
}

//------------------------------------------------------------------------------
/**
*/
inline
Blob::Blob(const Blob& rhs) :
	ptr(0),
	size(0),
	allocSize(0)
{
	if (rhs.IsValid())
	{
		this->Copy(rhs.ptr, rhs.size);
	}
}

//------------------------------------------------------------------------------
/**
*/
inline
Blob::Blob(Blob&& rhs) :
    ptr(0),
    size(0),
    allocSize(0)
{
    if (rhs.IsValid())
    {
        this->ptr = rhs.ptr;
        this->size = rhs.size;
        this->allocSize = rhs.allocSize;
        rhs.ptr = nullptr;
        rhs.size = 0;
        rhs.allocSize = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
Blob::Blob(SizeT s) :
	ptr(0),
	size(0),
	allocSize(0)
{
	this->Allocate(s);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Blob::IsValid() const
{
	return (0 != this->ptr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Delete()
{
	if (this->IsValid())
	{
		n_assert(0 != DataHeap);
		DataHeap->Free((void*)this->ptr);
		this->ptr = 0;
		this->size = 0;
		this->allocSize = 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
inline
Blob::~Blob()
{
	this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Allocate(SizeT s)
{
	n_assert(!this->IsValid());
	n_assert(0 != DataHeap);
	this->ptr = DataHeap->Alloc(s);
	this->allocSize = s;
	this->size = s;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Copy(const void* fromPtr, SizeT fromSize)
{
	n_assert((0 != fromPtr) && (fromSize > 0));

	// only re-allocate if not enough space
	if ((0 == this->ptr) || (this->allocSize < fromSize))
	{
		this->Delete();
		this->Allocate(fromSize);
	}
	this->size = fromSize;
	Memory::Copy(fromPtr, (void*)this->ptr, fromSize);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::GrowTo(SizeT size)
{
	n_assert(this->allocSize < size);

	void* newPtr = DataHeap->Alloc(size);
	Memory::Copy(this->ptr, newPtr, this->size);

	this->Delete();

	this->ptr = newPtr;
	this->allocSize = size;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::operator=(const Blob& rhs)
{
    if (rhs.IsValid())
    {
        this->Copy(rhs.ptr, rhs.size);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::operator=(Blob&& rhs)
{
    if (this != &rhs)
    {
        if (this->IsValid())
        {
            this->Delete();
        }
        this->ptr = rhs.ptr;
        this->size = rhs.size;
        this->allocSize = rhs.allocSize;
        rhs.ptr = nullptr;
        rhs.size = 0;
        rhs.allocSize = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Blob::operator==(const Blob& rhs) const
{
    return (this->BinaryCompare(rhs) == 0);
}
            
//------------------------------------------------------------------------------
/**
*/
inline bool
Blob::operator!=(const Blob& rhs) const
{
    return (this->BinaryCompare(rhs) != 0);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Blob::operator>(const Blob& rhs) const
{
    return (this->BinaryCompare(rhs) > 0);
}
            
//------------------------------------------------------------------------------
/**
*/
inline bool
Blob::operator<(const Blob& rhs) const
{
    return (this->BinaryCompare(rhs) < 0);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Blob::operator>=(const Blob& rhs) const
{
    return (this->BinaryCompare(rhs) >= 0);
}
            
//------------------------------------------------------------------------------
/**
*/
inline bool
Blob::operator<=(const Blob& rhs) const
{
    return (this->BinaryCompare(rhs) <= 0);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Reserve(SizeT s)
{
    if (this->allocSize > 0 && this->allocSize < s)
    {
        this->Delete();        
    }
    this->Allocate(s);
    this->size = s;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Trim(SizeT trimSize)
{
    n_assert(trimSize <= this->size);
    this->size = trimSize;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::Set(const void* fromPtr, SizeT fromSize)
{
    this->Copy(fromPtr, fromSize);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Blob::SetChunk(const void* from, SizeT size, SizeT internalOffset)
{
	n_assert((0 != from) && (size > 0));
	n_assert(nullptr != this->ptr)

	SizeT newSize = (internalOffset + size);
	if (newSize > this->allocSize)
	{
		this->GrowTo(newSize);
	}
	
	Memory::Copy(from, (void*)((byte*)this->ptr + internalOffset), size);
}

//------------------------------------------------------------------------------
/**
*/
inline void*
Blob::GetPtr() const
{
    n_assert(this->IsValid());
    return this->ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Blob::Size() const
{
    n_assert(this->IsValid());
    return this->size;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
Blob::HashCode() const
{
    IndexT hash = 0;
    const char* charPtr = (const char*) this->ptr;
    IndexT i;
    for (i = 0; i < this->size; i++)
    {
        hash += charPtr[i];
        hash += hash << 10;
        hash ^= hash >>  6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    hash &= ~(1<<31);       // don't return a negative number (in case IndexT is defined signed)
    return hash;
}

} // namespace Util
//------------------------------------------------------------------------------
    