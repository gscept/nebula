#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilderClip
    
    An animation curve for the N3 AnimBuilder class.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/stringatom.h"
#include "coreanimation/infinitytype.h"
#include "coreanimation/animevent.h"
#include "timing/time.h"
#include "toolkitutil/animutil/animbuildercurve.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimBuilderClip
{
public:
    /// constructor
    AnimBuilderClip();

    /// reserve the number of animation curves
    void ReserveCurves(SizeT numCurves);

    /// set name of clip
    void SetName(const Util::StringAtom& n);
    /// get name of clip
    const Util::StringAtom& GetName() const;
    /// set the pre-infinity type
    void SetPreInfinityType(CoreAnimation::InfinityType::Code preInfinityType);
    /// get the pre-infinity type
    CoreAnimation::InfinityType::Code GetPreInfinityType() const;
    /// set the post-infinity type
    void SetPostInfinityType(CoreAnimation::InfinityType::Code postInfinityType);
    /// get the post-infinity type
    CoreAnimation::InfinityType::Code GetPostInfinityType() const;
    /// set number of keys in clip
    void SetNumKeys(SizeT numKeys);
    /// get number of keys in clip
    SizeT GetNumKeys() const;
    /// set key stride
    void SetKeyStride(SizeT keyStride);
    /// get key stride
    SizeT GetKeyStride() const;
    /// set start key index (actual start time of the curve)
    void SetStartKeyIndex(IndexT keyIndex);
    /// get start key index
    IndexT GetStartKeyIndex() const;
    /// set the duration of a key in ticks
    void SetKeyDuration(Timing::Tick d);
    /// get the duration of a key
    Timing::Tick GetKeyDuration() const;    

    /// add a curve to the clip
    void AddCurve(const AnimBuilderCurve& curve);
    /// insert a curve at given index
    void InsertCurve(IndexT index, const AnimBuilderCurve& curve);
    /// get number of curves in the clip
    SizeT GetNumCurves() const;
    /// get curve at index
    AnimBuilderCurve& GetCurveAtIndex(IndexT i);

    /// add an event to the clip
    void AddEvent(const CoreAnimation::AnimEvent& animEvent);
    /// get number of anim events
    SizeT GetNumAnimEvents() const;
    /// get anim event at index
    CoreAnimation::AnimEvent& GetAnimEventAtIndex(IndexT i);

private:
    Util::Array<AnimBuilderCurve> curveArray;
    Util::Array<CoreAnimation::AnimEvent> eventArray;
    Util::StringAtom name;
    IndexT startKeyIndex;
    SizeT numKeys;
    SizeT keyStride;
    Timing::Tick keyDuration;
    CoreAnimation::InfinityType::Code preInfinityType;
    CoreAnimation::InfinityType::Code postInfinityType;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
AnimBuilderClip::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetPreInfinityType(CoreAnimation::InfinityType::Code t)
{
    this->preInfinityType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreAnimation::InfinityType::Code
AnimBuilderClip::GetPreInfinityType() const
{
    return this->preInfinityType;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetPostInfinityType(CoreAnimation::InfinityType::Code t)
{
    this->postInfinityType = t;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreAnimation::InfinityType::Code
AnimBuilderClip::GetPostInfinityType() const
{
    return this->postInfinityType;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetNumKeys(SizeT n)
{
    this->numKeys = n;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimBuilderClip::GetNumKeys() const
{
    return this->numKeys;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetKeyStride(SizeT s)
{
    this->keyStride = s;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimBuilderClip::GetKeyStride() const
{
    return this->keyStride;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetStartKeyIndex(IndexT i)
{
    this->startKeyIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AnimBuilderClip::GetStartKeyIndex() const
{
    return this->startKeyIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetKeyDuration(Timing::Tick d)
{
    this->keyDuration = d;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimBuilderClip::GetKeyDuration() const
{
    return this->keyDuration;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimBuilderClip::GetNumCurves() const
{
    return this->curveArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline AnimBuilderCurve&
AnimBuilderClip::GetCurveAtIndex(IndexT i)
{
    return this->curveArray[i];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimBuilderClip::GetNumAnimEvents() const
{
    return this->eventArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline CoreAnimation::AnimEvent&
AnimBuilderClip::GetAnimEventAtIndex(IndexT i)
{
    return this->eventArray[i];
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    