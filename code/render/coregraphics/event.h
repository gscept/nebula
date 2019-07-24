#pragma once
//------------------------------------------------------------------------------
/**
	An event is a signal and wait type object which is used for in-queue GPU sync.

	(C) 2017-2018 Individual contributors, see AUTHORS file
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
	BarrierStage leftDependency;
	BarrierStage rightDependency;
	Util::Array<std::tuple<RenderTextureId, ImageSubresourceInfo, CoreGraphicsImageLayout, CoreGraphicsImageLayout, BarrierAccess, BarrierAccess>> renderTextures;
	Util::Array<std::tuple<ShaderRWBufferId, BarrierAccess, BarrierAccess>> shaderRWBuffers;
	Util::Array<std::tuple<ShaderRWTextureId, ImageSubresourceInfo, CoreGraphicsImageLayout, CoreGraphicsImageLayout, BarrierAccess, BarrierAccess>> shaderRWTextures;
};

/// create new event
EventId CreateEvent(const EventCreateInfo& info);
/// destroy even
void DestroyEvent(const EventId id);

/// insert event in command buffer to be signaled
void EventSignal(const EventId id, const CoreGraphicsQueueType queue);
/// insert wait event in command buffer to wait for
void EventWait(const EventId id, const CoreGraphicsQueueType queue);
/// insert reset event
void EventReset(const EventId id, const CoreGraphicsQueueType queue);
/// insert both a wait and reset
void EventWaitAndReset(const EventId id, const CoreGraphicsQueueType queue);

} // CoreGraphics

#pragma pop_macro("CreateEvent")
