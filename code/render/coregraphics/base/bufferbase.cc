//------------------------------------------------------------------------------
// bufferbase.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "bufferbase.h"

namespace Base
{

__ImplementClass(Base::BufferBase, 'BBAS', Base::GpuResourceBase);
//------------------------------------------------------------------------------
/**
*/
BufferBase::BufferBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BufferBase::~BufferBase()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void
BufferBase::CreateLock()
{
	this->lock = CoreGraphics::BufferLock::Create();
}


//------------------------------------------------------------------------------
/**
*/
void
BufferBase::DestroyLock()
{
	this->lock = 0;
}

} // namespace Base