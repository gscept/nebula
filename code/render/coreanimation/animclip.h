#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimClip
  
    An animation clip is a collection of related animation curves (for
    instance all curves required to animate a character).

    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "coreanimation/infinitytype.h"
#include "coreanimation/animcurve.h"
#include "coreanimation/animevent.h"
#include "util/stringatom.h"
#include "timing/time.h"
#include "util/dictionary.h"
#include "util/fixedarray.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimClip
{
public:
    /// constructor
    AnimClip();

    /// set the name of the clip
    void SetName(const Util::StringAtom& n);
    /// get the name of the clip
    const Util::StringAtom& GetName() const;
    /// set the number of keys per animation curve in the clip
    void SetNumKeys(SizeT numKeys);
    /// get the number of keys per animation curve in the clip
    SizeT GetNumKeys() const;
    /// set the key stride (number of float's between keys of the same curve)
    void SetKeyStride(SizeT stride);
    /// get the key stride
    SizeT GetKeyStride() const;
    /// set the duration of a key in ticks
    void SetKeyDuration(Timing::Tick d);
    /// get the duration of a key
    Timing::Tick GetKeyDuration() const;
    /// get the clip duration in ticks
    Timing::Tick GetClipDuration() const;
    /// set the pre-infinity type
    void SetPreInfinityType(InfinityType::Code preInfinityType);
    /// get the pre-infinity type
    InfinityType::Code GetPreInfinityType() const;
    /// set the post-infinity type
    void SetPostInfinityType(InfinityType::Code postInfinityType);
    /// get the post-infinity type
    InfinityType::Code GetPostInfinityType() const;
    /// set start key index (actual start time of the curve)
    void SetStartKeyIndex(IndexT keyIndex);
    /// get start key index
    IndexT GetStartKeyIndex() const;
    /// get start time
    Timing::Tick GetStartTime() const;

    /// set number of curves in clip
    void SetNumCurves(SizeT numCurves);
    /// get number of curves in clip
    SizeT GetNumCurves() const;
    /// access to curve by index
    AnimCurve& CurveByIndex(IndexT curveIndex) const;

    /// begin adding events
    void BeginEvents(SizeT numEvents);
    /// add an event
    void AddEvent(const AnimEvent& event);
    /// end adding events
    void EndEvents();
    /// get number of events in clip
    SizeT GetNumEvents() const;
    /// return true if event exists by name
    bool HasEvent(const Util::StringAtom& name) const;
    /// get event by name
    const AnimEvent& GetEventByName(const Util::StringAtom& name) const;
    /// get event by index
    const AnimEvent& GetEventByIndex(IndexT i) const;
    /// get events in time range (return number of events and start event index)
	SizeT GetEventsInRange(Timing::Tick startTime, Timing::Tick endTime, IndexT& outStartEventIndex) const;

    /// pre-compute the key range values
    void PrecomputeKeySliceValues();
    /// return true if PrecomputeKeySliceValues had been called
    bool AreKeySliceValuesValid() const;
    /// get index of first key in clip's key range (this is the key index of the first non-static curve)
    IndexT GetKeySliceFirstKeyIndex() const;
    /// get byte size of a key slize in the clip
    SizeT GetKeySliceByteSize() const;

private:
    Util::StringAtom name;
    IndexT startKeyIndex;
    SizeT numKeys;
    SizeT keyStride;
    Timing::Tick keyDuration;
    InfinityType::Code preInfinityType;
    InfinityType::Code postInfinityType;
    Util::FixedArray<AnimCurve> curves;
    Util::Array<AnimEvent> events;
    Util::Dictionary<Util::StringAtom, IndexT> eventIndexMap;
    IndexT keySliceFirstKeyIndex;       // pre-computed in SetupKeyRange()
    SizeT keySliceByteSize;             // pre-computed in SetupKeyRange()
    bool keySliceValuesValid;
    bool inBeginEvents;
};

//------------------------------------------------------------------------------
/**
    NOTE: if all curves in the clip are static, then this method will return
    InvalidIndex, this is not an error situation!
*/
inline IndexT
AnimClip::GetKeySliceFirstKeyIndex() const
{
    return this->keySliceFirstKeyIndex;
}

//------------------------------------------------------------------------------
/**
    NOTE: if all curves in the clip are static, then this method will
    return 0, this is not an error situation!
*/
inline SizeT
AnimClip::GetKeySliceByteSize() const
{
    return this->keySliceByteSize;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimClip::AreKeySliceValuesValid() const
{
    return this->keySliceValuesValid;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
AnimClip::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetNumKeys(SizeT n)
{
    this->numKeys = n;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimClip::GetNumKeys() const
{
    return this->numKeys;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetKeyStride(SizeT s)
{
    this->keyStride = s;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimClip::GetKeyStride() const
{
    return this->keyStride;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetKeyDuration(Timing::Tick d)
{
    this->keyDuration = d;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimClip::GetKeyDuration() const
{
    return this->keyDuration;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimClip::GetClipDuration() const
{
    return this->numKeys * this->keyDuration;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetPreInfinityType(InfinityType::Code t)
{
    this->preInfinityType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline InfinityType::Code
AnimClip::GetPreInfinityType() const
{
    return this->preInfinityType;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetPostInfinityType(InfinityType::Code t)
{
    this->postInfinityType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline InfinityType::Code
AnimClip::GetPostInfinityType() const
{
    return this->postInfinityType;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimClip::GetNumEvents() const
{
    return this->events.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const AnimEvent&
AnimClip::GetEventByIndex(IndexT i) const
{
    return this->events[i];
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetStartKeyIndex(IndexT i)
{
    this->startKeyIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AnimClip::GetStartKeyIndex() const
{
    return this->startKeyIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimClip::GetStartTime() const
{
    return this->startKeyIndex * this->keyDuration;
}

} // namespace CoreAnimation
//------------------------------------------------------------------------------
