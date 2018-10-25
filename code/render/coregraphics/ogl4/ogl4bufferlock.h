#pragma once
//------------------------------------------------------------------------------
/**
	@class OpenGL4::OGL4BufferLock
	
	A buffer lock in OpenGL4 uses a range of GLSync objects which gets locked and unlocked whenever a fragment of data is required.
	
	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/bufferlockbase.h"
namespace OpenGL4
{
class OGL4BufferLock : public Base::BufferLockBase
{
	__DeclareClass(OGL4BufferLock);
public:
	/// constructor
	OGL4BufferLock();
	/// destructor
	virtual ~OGL4BufferLock();

	/// lock a buffer
	void LockBuffer(IndexT index);
	/// wait for a buffer to finish
	void WaitForBuffer(IndexT index);
	/// lock a range
	void LockRange(IndexT startIndex, SizeT length);
	/// wait for a locked range
	void WaitForRange(IndexT startIndex, SizeT length);
private:
	/// perform waiting
	void Wait(GLsync sync);
	/// cleanup
	void Cleanup(GLsync sync);

	Util::Dictionary<Base::BufferRange, GLsync> rangeLocks;
	Util::Dictionary<IndexT, GLsync> bufferLocks;
};
} // namespace OpenGL4