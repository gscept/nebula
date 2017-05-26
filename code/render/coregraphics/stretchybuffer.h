#pragma once
//------------------------------------------------------------------------------
/**
	A stretchy buffer implements a GPU memory pool, which uses the same buffer
	and expands its memory backing as needed. Implement the Grow function
	in the implementation class to perform the resize.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace CoreGraphics
{
class StretchyBuffer : public Core::RefCounted
{
	__DeclareAbstractClass(StretchyBuffer);
public:
	/// constructor
	StretchyBuffer();
	/// destructor
	virtual ~StretchyBuffer();

	/// set all free indices to be considered used up
	void SetFull();
	/// set range of values to be considered free
	void SetFree(uint start, SizeT num);
	/// allocates instance memory, and returns offset into buffer at new instance
	SizeT AllocateInstance(SizeT numInstances = 1);
	/// deallocates instance memory
	void FreeInstance(SizeT offset);
protected:

	/// grow uniform buffer, returns new aligned size
	virtual uint32_t Grow(SizeT oldCapacity, SizeT growBy) = 0;

	Util::Array<IndexT> freeIndices;
	Util::Array<IndexT> usedIndices;
	SizeT grow;
	SizeT stride;
	uint size;
	uint byteAlignment;
};
} // namespace CoreGraphics