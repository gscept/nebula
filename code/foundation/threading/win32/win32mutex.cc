//------------------------------------------------------------------------------
//  win32mutex.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "threading/mutex.h"
#include "core/debug.h"

namespace Threading
{

//------------------------------------------------------------------------------
/**
*/
Mutex::Mutex()
{
	this->handle = CreateMutex(nullptr, false, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
Mutex::~Mutex()
{
	CloseHandle(this->handle);
}

//------------------------------------------------------------------------------
/**
*/
void
Mutex::Acquire()
{
	DWORD result = WaitForSingleObject(this->handle, INFINITE);
	n_assert(result == WAIT_OBJECT_0);
}

//------------------------------------------------------------------------------
/**
*/
void
Mutex::Release()
{
	BOOL res = ReleaseMutex(this->handle);
	n_assert(res == true);
}

} // namespace Threading
