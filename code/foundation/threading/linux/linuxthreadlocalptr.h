#pragma once
//------------------------------------------------------------------------------
/**
 * 	@class Linux::LinuxThreadLocalPtr
 *
 * 	A thread-local pointer class for platforms which don't have proper
 * 	support for the __thread keyword.
 *
 * 	(C) A.Weissflog 2011
 *  (C) 2013-2018 Individual contributors, see AUTHORS file
 */
#include "threading/linux/linuxthreadlocaldata.h"

//------------------------------------------------------------------------------
namespace Linux
{
class LinuxThreadLocalPtr
{
public:
	/// constructor
	LinuxThreadLocalPtr();
	/// set pointer
	void Set(void* ptr);
	/// get pointer
	void* Get() const;
	/// clear the pointer
	void Clear();

private:
	IndexT slot;
};

//------------------------------------------------------------------------------
/**
 */
inline void
LinuxThreadLocalPtr::Set(void* ptr)
{
	LinuxThreadLocalData::SetPointer(this->slot, ptr);
}

//------------------------------------------------------------------------------
/**
 */
inline void*
LinuxThreadLocalPtr::Get() const
{
	return LinuxThreadLocalData::GetPointer(this->slot);
}

//------------------------------------------------------------------------------
/**
 */
inline void
LinuxThreadLocalPtr::Clear()
{
	LinuxThreadLocalData::ClearPointer(this->slot);
}

} // namespace Linux
//------------------------------------------------------------------------------
