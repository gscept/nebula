#pragma once
//------------------------------------------------------------------------------
/**
	An event is a signal and wait type object which is used for in-queue GPU sync.

	(C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "coregraphics/barrier.h"
#include "coregraphics/commandbuffer.h"
#include "coregraphics/config.h"
#include "util/stringatom.h"

#ifdef CreateEvent
#pragma push_macro("CreateEvent")
#undef CreateEvent
#endif
namespace CoreGraphics
{

ID_24_8_TYPE(EventId);

struct EventCreateInfo
{
	Util::StringAtom name;
	bool createSignaled : 1;
	Util::Array<TextureBarrier> textures;
	Util::Array<BufferBarrier> rwBuffers;
	Util::Array<ExecutionBarrier> barriers;
};

/// create new event
EventId CreateEvent(const EventCreateInfo& info);
/// destroy even
void DestroyEvent(const EventId id);

/// insert event in command buffer to be signaled
void EventSignal(const EventId id, const CoreGraphics::BarrierStage stage, const CoreGraphics::QueueType queue);
/// insert wait event in command buffer to wait for
void EventWait(
	const EventId id, 
	const CoreGraphics::BarrierStage waitStage,
	const CoreGraphics::BarrierStage signalStage,
	const CoreGraphics::QueueType queue);
/// insert reset event
void EventReset(const EventId id, const CoreGraphics::BarrierStage stage, const CoreGraphics::QueueType queue);
/// insert both a wait and reset
void EventWaitAndReset(const EventId id, const CoreGraphics::BarrierStage waitStage, const CoreGraphics::BarrierStage signalStage, const CoreGraphics::QueueType queue);

/// get event status on host
bool EventPoll(const EventId id);
/// unset event on host
void EventHostReset(const EventId id);
/// signal event on host
void EventHostSignal(const EventId id);

} // CoreGraphics

#pragma pop_macro("CreateEvent")
