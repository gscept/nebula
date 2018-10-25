#pragma once
//------------------------------------------------------------------------------
/**
	@class Base::BufferLockBase
	
	A buffer lock is used to avoid writing to a GPU buffer while its in flight.
	Also defines a struct which is used to represent a buffer range.
	
	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace Base
{
class BufferLockBase : public Core::RefCounted
{
	__DeclareClass(BufferLockBase);
public:

	/// constructor
	BufferLockBase();
	/// destructor
	virtual ~BufferLockBase();

	/// lock a buffer
	void LockBuffer(IndexT index);
	/// wait for a buffer to finish
	void WaitForBuffer(IndexT index);
	/// lock a range
	void LockRange(IndexT startIndex, SizeT length);
	/// wait for a locked range
	void WaitForRange(IndexT startIndex, SizeT length);
};

struct BufferRange
{
	IndexT start;
	SizeT length;

	/// checks for overlaps with other ranges
	inline bool
	Overlaps(const BufferRange& rhs) const
	{
		return this->start < (rhs.start + rhs.length) && rhs.start < (this->start + this->length);
	}
};

//------------------------------------------------------------------------------
/**
*/
inline bool
operator!=(const BufferRange& lhs, const BufferRange& rhs)
{
	return lhs.start != rhs.start || lhs.length != rhs.length;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>(const BufferRange& lhs, const BufferRange& rhs)
{
	return lhs.start > rhs.start;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator<(const BufferRange& lhs, const BufferRange& rhs)
{
	return lhs.start < rhs.start;
}

} // namespace Base