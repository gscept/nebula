//------------------------------------------------------------------------------
//  bufferlockbase.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "bufferlockbase.h"

namespace Base
{
__ImplementClass(Base::BufferLockBase, 'BULB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
BufferLockBase::BufferLockBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BufferLockBase::~BufferLockBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
BufferLockBase::LockBuffer(IndexT index)
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
BufferLockBase::WaitForBuffer(IndexT index)
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
BufferLockBase::LockRange(IndexT startIndex, SizeT length)
{
	// override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
BufferLockBase::WaitForRange(IndexT startIndex, SizeT length)
{
	// override in subclass
}


} // namespace Base