#pragma once
//------------------------------------------------------------------------------
/**
	An event is a signal and wait type object which is used for in-queue GPU sync.

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "coregraphics/barrier.h"
#include "coregraphics/cmdbuffer.h"

#ifdef CreateEvent
#pragma push_macro("CreateEvent")
#undef CreateEvent
#endif
namespace CoreGraphics
{

ID_24_8_TYPE(EventId);

struct EventCreateInfo
{
	bool createSignaled : 1;

	BarrierDependency leftDependency;
	BarrierDependency rightDependency;
	Util::Array<std::tuple<RenderTextureId, BarrierAccess, BarrierAccess>> renderTextureBarriers;
	Util::Array<std::tuple<ShaderRWBufferId, BarrierAccess, BarrierAccess>> shaderRWBuffers;
	Util::Array<std::tuple<ShaderRWTextureId, BarrierAccess, BarrierAccess>> shaderRWTextures;
};

/// create new event
EventId CreateEvent(const EventCreateInfo& info);
/// destroy even
void DestroyEvent(const EventId id);

/// insert event in command buffer to be signaled
void Signal(const EventId id, const CmdBufferId cmd, const BarrierDependency when);
/// insert wait event in command buffer to wait for
void Wait(const EventId id, const CmdBufferId cmd);
/// insert reset event
void Reset(const EventId id, const CmdBufferId cmd, const BarrierDependency when);

} // CoreGraphics

#pragma pop_macro("CreateEvent")
