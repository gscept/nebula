//------------------------------------------------------------------------------
//  ogl4bufferlock.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4bufferlock.h"

using namespace Base;
namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4BufferLock, 'O4BL', Base::BufferLockBase);

//------------------------------------------------------------------------------
/**
*/
OGL4BufferLock::OGL4BufferLock()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4BufferLock::~OGL4BufferLock()
{
	IndexT i;
	for (i = 0; i < this->rangeLocks.Size(); i++)
	{
		this->Wait(this->rangeLocks.ValueAtIndex(i));
		this->Cleanup(this->rangeLocks.ValueAtIndex(i));
	}
	this->rangeLocks.Clear();
	for (i = 0; i < this->bufferLocks.Size(); i++)
	{
		this->Wait(this->bufferLocks.ValueAtIndex(i));
		this->Cleanup(this->bufferLocks.ValueAtIndex(i));
	}
	this->bufferLocks.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4BufferLock::LockRange(IndexT startIndex, SizeT length)
{
	BufferRange range = { startIndex, length };
	GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	this->rangeLocks.Add(range, sync);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4BufferLock::WaitForRange(IndexT startIndex, SizeT length)
{
	BufferRange range = { startIndex, length };
	IndexT i;
	for (i = 0; i < this->rangeLocks.Size(); i++)
	{
		const Util::KeyValuePair<BufferRange, GLsync>& kvp = this->rangeLocks.KeyValuePairAtIndex(i);
		if (range.Overlaps(kvp.Key()))
		{
			this->Wait(kvp.Value());
			this->Cleanup(kvp.Value());
			this->rangeLocks.Erase(kvp.Key());
			i--;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4BufferLock::LockBuffer(IndexT index)
{
	if (!this->bufferLocks.Contains(index))
	{
		GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		this->bufferLocks.Add(index, sync);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4BufferLock::WaitForBuffer(IndexT index)
{
	if (this->bufferLocks.Contains(index))
	{
		this->Wait(this->bufferLocks[index]);
		this->Cleanup(this->bufferLocks[index]);
		this->bufferLocks.Erase(index);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4BufferLock::Wait(GLsync sync)
{
    GLbitfield waitFlags = 0;
	GLuint64 waitDuration = 0;
	while (true)
	{
        GLenum waitRet = glClientWaitSync(sync, waitFlags, waitDuration);
		if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED) return;
		if (waitRet == GL_WAIT_FAILED)
		{
			n_error("OGL4BufferLock::Wait(): Waiting for buffer lock failed, this should never happen unless your graphics card is b0rked.");
			return;
		}

		// After the first time, need to start flushing, and wait for a looong time.
        waitFlags = GL_SYNC_FLUSH_COMMANDS_BIT;
		waitDuration = 1000000000;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4BufferLock::Cleanup(GLsync sync)
{
	glDeleteSync(sync);
}

} // namespace OpenGL4