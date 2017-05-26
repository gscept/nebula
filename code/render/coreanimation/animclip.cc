//------------------------------------------------------------------------------
//  animclip.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coreanimation/animclip.h"

namespace CoreAnimation
{
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimClip::AnimClip() :
    startKeyIndex(0),
    numKeys(0),
    keyStride(0),
    keyDuration(0),
    preInfinityType(InfinityType::Constant),
    postInfinityType(InfinityType::Constant),
    keySliceFirstKeyIndex(InvalidIndex),
    keySliceByteSize(0),
    keySliceValuesValid(false),
    inBeginEvents(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AnimClip::SetNumCurves(SizeT numCurves)
{
    this->curves.SetSize(numCurves);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
AnimClip::GetNumCurves() const
{
    return this->curves.Size();
}

//------------------------------------------------------------------------------
/**
*/
AnimCurve&
AnimClip::CurveByIndex(IndexT curveIndex) const
{
    return this->curves[curveIndex];
}

//------------------------------------------------------------------------------
/**
*/
void
AnimClip::BeginEvents(SizeT numEvents)
{
    n_assert(!this->inBeginEvents);
    n_assert(this->events.IsEmpty());
    this->inBeginEvents = true;
    if (numEvents > 0)
    {
        this->events.Reserve(numEvents);
        this->eventIndexMap.Reserve(numEvents);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimClip::AddEvent(const AnimEvent& animEvent)
{
    n_assert(this->inBeginEvents);
    n_assert(animEvent.GetName().IsValid());
    this->events.Append(animEvent);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimClip::EndEvents()
{
    n_assert(this->inBeginEvents);
    this->inBeginEvents = false;
    if (this->events.Size() > 0)
    {
        // sort event array by event time
        this->events.Sort();

        // build name-lookup table
        IndexT i;
        this->eventIndexMap.BeginBulkAdd();
        for (i = 0; i < this->events.Size(); i++)
        {
            this->eventIndexMap.Add(this->events[i].GetName(), i);
        }
        this->eventIndexMap.EndBulkAdd();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
AnimClip::HasEvent(const StringAtom& name) const
{
    return this->eventIndexMap.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
const AnimEvent&
AnimClip::GetEventByName(const StringAtom& name) const
{
    return this->events[this->eventIndexMap[name]];
}

//------------------------------------------------------------------------------
/**
    Precompute the 2 key slice values (first key index and key slice size).
    A key slice is the memory range of all curve-keys at a given 
    key index. The numbers must be pre-computed because only non-static
    curves have keys in the key-slice.
*/
void
AnimClip::PrecomputeKeySliceValues()
{
    this->keySliceValuesValid = true;

    // find index of first key in key slice, and compute the
    // byte size of one key slice
    IndexT curveIndex;
    for (curveIndex = 0; curveIndex < this->curves.Size(); curveIndex++)
    {
        const AnimCurve& curve = this->curves[curveIndex];
        if (!curve.IsStatic())
        {
            if (InvalidIndex == this->keySliceFirstKeyIndex)
            {
                this->keySliceFirstKeyIndex = curve.GetFirstKeyIndex();
            }
            this->keySliceByteSize += sizeof(float4);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Get events in a specific time range. Return the number of events in the
    time range, and the index of the start event. This does a linear search
    on the event array.
*/
SizeT
AnimClip::GetEventsInRange(Timing::Tick startTime, Timing::Tick endTime, IndexT& outStartEventIndex) const
{
    outStartEventIndex = InvalidIndex;

    // find start index
    SizeT numEvents = this->events.Size();

    // skip all events which lay before starttime
    IndexT i = 0;
	while ((i < numEvents) && (startTime > this->events[i].GetTime()))
    {
        i++;
    }

    if (i < numEvents)
    {
        outStartEventIndex = i;
        SizeT numEventsInRange = 0;
        // check if event is before endtime
		while ((i < numEvents) && (endTime > this->events[i].GetTime()))
        {
            numEventsInRange++;
            i++;
        }
        return numEventsInRange;
    }
    return 0;
}

} // namespace CoreAnimation
