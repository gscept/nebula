//------------------------------------------------------------------------------
//  readwritelock.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "readwritelock.h"
namespace Threading
{

//------------------------------------------------------------------------------
/**
*/
ReadWriteLock::ReadWriteLock()
	: numReaders(0)
	, writerThread(InvalidThreadId)
{
}

//------------------------------------------------------------------------------
/** 
*/
ReadWriteLock::~ReadWriteLock()
{
}

//------------------------------------------------------------------------------
/**
*/
void
ReadWriteLock::Acquire(const RWAccessFlags accessFlags)
{
	if (AllBits(accessFlags, ReadAccess))
	{
		// wait here for any writes to finish
		this->readerSection.Enter();
		this->writerSection.Enter();

		// the mutex stops the increment from happening non-atomically
		// but in the Release function
		this->numReaders++;

		// give back the mutex
		this->writerSection.Leave();
		this->readerSection.Leave();
	}

	if (AllBits(accessFlags, WriteAccess))
	{
		this->writerSection.Enter();
		while (this->numReaders > 0)
			Thread::YieldThread();

		ThreadId myThread = Threading::Thread::GetMyThreadId();
		this->writerThread = myThread;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ReadWriteLock::Release(const RWAccessFlags accessFlags)
{
	if (AllBits(accessFlags, ReadAccess))
	{
		this->readerSection.Enter();
		n_assert(this->numReaders != 0);
		this->numReaders--;
		this->readerSection.Leave();
	}

	if (AllBits(accessFlags, WriteAccess))
	{
		ThreadId myThread = Threading::Thread::GetMyThreadId();
		n_assert(this->writerThread == myThread);
		this->writerSection.Leave();
	}
}

} // namespace Threading
