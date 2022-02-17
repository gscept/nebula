#pragma once
//------------------------------------------------------------------------------
/**
    An event is a signal and wait type object which is used for in-queue GPU sync.

    @copyright
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
    CoreGraphics::PipelineStage fromStage, toStage;
    Util::Array<TextureBarrierInfo> textures;
    Util::Array<BufferBarrierInfo> buffers;
};

/// create new event
EventId CreateEvent(const EventCreateInfo& info);
/// destroy even
void DestroyEvent(const EventId id);

/// insert event in command buffer to be signaled
void EventSignal(const EventId id, const CoreGraphics::CmdBufferId buf, const CoreGraphics::PipelineStage stage);
/// insert event in queue to wait for
void EventWait(
    const EventId id, 
    const CoreGraphics::QueueType queue,
    const CoreGraphics::PipelineStage waitStage,
    const CoreGraphics::PipelineStage signalStage
    );
/// insert event in command buffer to wait for
void EventWait(
    const EventId id,
    const CoreGraphics::CmdBufferId buf,
    const CoreGraphics::PipelineStage waitStage,
    const CoreGraphics::PipelineStage signalStage
);
/// insert reset event
void EventReset(const EventId id, const CoreGraphics::CmdBufferId buf, const CoreGraphics::PipelineStage stage);
/// insert both a wait and reset
void EventWaitAndReset(const EventId id, const CoreGraphics::CmdBufferId buf, const CoreGraphics::PipelineStage waitStage, const CoreGraphics::PipelineStage signalStage);

/// get event status on host
bool EventPoll(const EventId id);
/// unset event on host
void EventHostReset(const EventId id);
/// signal event on host
void EventHostSignal(const EventId id);
/// wait for event to be signaled on host
void EventHostWait(const EventId id);

} // CoreGraphics

#pragma pop_macro("CreateEvent")
