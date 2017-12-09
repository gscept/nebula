#pragma once
//------------------------------------------------------------------------------
/**
	An event is a signal and wait type object which is used for in-queue GPU sync.

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"

#ifdef CreateEvent
#undef CreateEvent
#endif
namespace CoreGraphics
{

ID_24_8_TYPE(EventId);

struct EventCreateInfo
{
	bool createSignaled : 1;
};

/// create new event
EventId CreateEvent(const EventCreateInfo& info);
/// destroy even
void DestroyEvent(const EventId id);
} // CoreGraphics
