#pragma once
//------------------------------------------------------------------------------
/**
	Allocates memory up-front, and then allows other systems to grab regions.
	Synchronizes used regions between frames (thread safe) in order to recycle
	previously occupied memory.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "threading/event.h"
#include "util/array.h"
namespace Memory
{

struct RingAlloc
{
	uint offset;
	byte* data;
};

template<uint SYNCPOINTS>
class RingAllocator
{
public:

	/// constructor
	RingAllocator(const SizeT size);
	/// destructor
	~RingAllocator();

	/// start allocation phase, checks sync points and unlocks regions if they have been signaled
	void Start();
	/// allocate memory
	bool Allocate(const SizeT size, RingAlloc& alloc);
	/// end allocation phase, insert sync points
	Threading::Event* End();
private:

	byte* buffer;
	uint size;
	Threading::Event ev[SYNCPOINTS];

	struct Interval
	{
		uint lower, upper;
	};

	enum SyncState
	{
		Signaled,
		Waiting,
		Reset
	};

	Util::FixedArray<Interval> lockedIntervals;
	Util::FixedArray<Threading::Event> events;
	Util::FixedArray<SyncState> states;
	Interval currentInterval;
	Interval freeInterval;
	uint currentAllocation;
	uint nextEvent;
};

//------------------------------------------------------------------------------
/**
*/
template<uint SYNCPOINTS>
inline
RingAllocator<SYNCPOINTS>::RingAllocator(const SizeT size) :
	size(size),
	nextEvent(0)
{
	this->lockedIntervals.Resize(SYNCPOINTS);
	this->events.Resize(SYNCPOINTS);
	this->states.Resize(SYNCPOINTS);

	for (uint i = 0; i < SYNCPOINTS; i++)
	{
		this->states[i] = Reset;
	}
	this->buffer = n_new_array(byte, size);
	
	this->currentAllocation = 0;
	this->currentInterval.lower = 0;
	this->currentInterval.upper = 0;
	this->freeInterval.lower = 0;
	this->freeInterval.upper = size;
}

//------------------------------------------------------------------------------
/**
*/
template<uint SYNCPOINTS>
inline
RingAllocator<SYNCPOINTS>::~RingAllocator()
{
	delete[] this->buffer;
}

//------------------------------------------------------------------------------
/**
*/
template<uint SYNCPOINTS>
inline
void RingAllocator<SYNCPOINTS>::Start()
{
	// reset intervals and counter
	this->currentAllocation = 0;
	this->currentInterval.lower = this->currentInterval.upper = this->freeInterval.lower;

	// get event and wait, they should be checked in order
	for (IndexT i = 0; i < this->events.Size(); i++)
	{
		Threading::Event& ev = this->events[i];
		if (ev.Peek())
		{
			// set interval and reset
			this->freeInterval.upper = this->lockedIntervals[i].upper;
			this->states[i] = Signaled;
			ev.Reset();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template<uint SYNCPOINTS>
inline bool
RingAllocator<SYNCPOINTS>::Allocate(const SizeT size, RingAlloc& alloc)
{
	n_assert(size <= this->size);
	uint begin = this->currentInterval.upper;
	uint end = begin + size;
	if (end > this->size && this->freeInterval.lower >= this->freeInterval.upper)
	{
		// wrap around to beginning, which should be okay if the lower is higher than upper, meaning 
		// we popped a lower interval during Start() and can now reuse from the beginning of the buffer
		alloc.offset = 0;
		end = this->freeInterval.upper;
	}
	else if (end > this->freeInterval.upper)
	{
		// if our intervals are not wrapping, we can just check to see if we will go overboard
		n_warning("Over-allocated RingAllocator!\n");
		alloc.data = nullptr;
		alloc.offset = -1;
		return false;
	}
	
	// set allocation structure
	alloc.data = this->buffer + begin;
	alloc.offset = begin;

	// move lower interval to the end for the next allocation
	this->currentInterval.upper = end;
	this->currentAllocation += size;
	return true;
}

//------------------------------------------------------------------------------
/**
*/
template<uint SYNCPOINTS>
inline Threading::Event*
RingAllocator<SYNCPOINTS>::End()
{
	Threading::Event* ret = nullptr;

	//  if we did allocate this frame, use one of the sync events
	if (this->currentAllocation > 0)
	{
		// save current interval as locked
		this->lockedIntervals[this->nextEvent] = this->currentInterval;
		this->states[this->nextEvent] = Waiting;
		ret = &this->events[this->nextEvent];
		this->nextEvent = (this->nextEvent + 1) % SYNCPOINTS;

		// set the free interval to be offset by the last allocation size
		this->freeInterval.lower = this->currentInterval.upper;
	}

	// return event so we can pass it to worker thread
	return ret;
}
} // namespace Memory
