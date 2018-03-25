//------------------------------------------------------------------------------
//  animeventemitter.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animeventemitter.h"

namespace CoreAnimation
{
//------------------------------------------------------------------------------
/**
    collects all animevents from given clip
*/
Util::Array<AnimEvent> 
CoreAnimation::AnimEventEmitter::EmitAnimEvents(const AnimClip& clip, Timing::Tick start, Timing::Tick end, bool isInfinite)
{
    Util::Array<AnimEvent> events;

    if (start <= end)
    {
        IndexT startIdx = 0;
		SizeT numEvents = clip.GetEventsInRange(start, end, startIdx);
        IndexT i;
        IndexT endIndex = startIdx + numEvents;
        for (i = startIdx; startIdx != InvalidIndex && i < endIndex; ++i)
        {
    	    events.Append(clip.GetEventByIndex(i));
        }
    }
    else
    {
        // from startCheckTime till end of clip
		Timing::Tick duration = clip.GetClipDuration();
		events.AppendArray(AnimEventEmitter::EmitAnimEvents(clip, start, duration, false));
        if (isInfinite)
        {
            // from start of clip till endCheckTime
			events.AppendArray(AnimEventEmitter::EmitAnimEvents(clip, 0, end, false));
        }
    }
    return events;
}
}