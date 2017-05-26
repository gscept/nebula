//------------------------------------------------------------------------------
//  shaderbufferbase.cc
//  (C) 2012-2014 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shaderreadwritebufferbase.h"
#include "../displaydevice.h"

namespace Base
{
__ImplementAbstractClass(Base::ShaderReadWriteBufferBase, 'SHBB', CoreGraphics::StretchyBuffer);

//------------------------------------------------------------------------------
/**
*/
ShaderReadWriteBufferBase::ShaderReadWriteBufferBase() :
	isSetup(false),
	lockSemaphore(0),
    bufferIndex(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderReadWriteBufferBase::~ShaderReadWriteBufferBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteBufferBase::Setup(const SizeT numBackingBuffers)
{
	n_assert(!this->isSetup);
	n_assert(this->size > 0);
	this->isSetup = true;
	this->numBuffers = numBackingBuffers;

	// set free buffers
	StretchyBuffer::SetFree(0, numBackingBuffers);

	if (this->relativeSize)
	{
		// get display mode
		Ptr<CoreGraphics::Window> wnd = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();
		const CoreGraphics::DisplayMode& mode = wnd->GetDisplayMode();
		this->byteSize = this->size * mode.GetWidth() * mode.GetHeight();
	}
	else
	{
		this->byteSize = this->size;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteBufferBase::Discard()
{
	n_assert(this->isSetup);
	this->isSetup = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteBufferBase::Update(void* data, uint offset, uint length)
{
	n_assert(offset < size);
	n_assert(length > 0);
	// implementation specific
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteBufferBase::CycleBuffers()
{
    this->bufferIndex = (this->bufferIndex + 1) % this->numBuffers;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteBufferBase::Lock()
{
	this->lockSemaphore++;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderReadWriteBufferBase::Unlock()
{
	this->lockSemaphore--;
}

} // namespace Base