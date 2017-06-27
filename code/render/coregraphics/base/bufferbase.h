#pragma once
//------------------------------------------------------------------------------
/**
	Description
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/gpuresourcebase.h"
#include "coregraphics/bufferlock.h"
namespace Base
{
class BufferBase : public GpuResourceBase
{
	__DeclareClass(BufferBase);
public:
	/// constructor
	BufferBase();
	/// destructor
	virtual ~BufferBase();

	/// create buffer lock
	void CreateLock();
	/// destroy lock
	void DestroyLock();

	/// set size in bytes
	void SetByteSize(const SizeT size);

	/// unlock range within index buffer
	void Unlock(SizeT offset, SizeT length);
	/// handle updating the buffer, passing an optional mapped data buffer if updates should go to it
	void Update(const void* data, SizeT offset, SizeT length, void* mappedData = NULL);
	/// lock range within index buffer
	void Lock(SizeT offset, SizeT length);
protected:
	SizeT byteSize;
	Ptr<CoreGraphics::BufferLock> lock;
};

//------------------------------------------------------------------------------
/**
*/
inline void
BufferBase::SetByteSize(const SizeT size)
{
	this->byteSize = size;
}

//------------------------------------------------------------------------------
/**
*/
inline void
BufferBase::Unlock(SizeT offset, SizeT length)
{
	n_assert(this->byteSize > offset + length);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BufferBase::Update(const void* data, SizeT offset, SizeT length, void* mappedData)
{
	n_assert(this->byteSize > offset + length);
}

//------------------------------------------------------------------------------
/**
*/
inline void
BufferBase::Lock(SizeT offset, SizeT length)
{
	n_assert(this->byteSize > offset + length);
}

} // namespace Base