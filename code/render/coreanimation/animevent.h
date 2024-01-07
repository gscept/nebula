#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimEvent
  
    An animation event associates a name with a point in time. The event
    will be triggered when the play-cursor passes the point in time of the
    event. AnimEvents are attached to anim clips.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/stringatom.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimEvent
{
public:
    /// constructor
    AnimEvent();
    /// constructor with name and time
    AnimEvent(const Util::StringAtom& name, Timing::Tick time);
    /// constructor with name, category and time
    AnimEvent(const Util::StringAtom& name, const Util::StringAtom& category, Timing::Tick time);

    /// equality operator (time only)
    friend bool operator==(const AnimEvent& a, const AnimEvent& b);
    /// inequality operator (time only)
    friend bool operator!=(const AnimEvent& a, const AnimEvent& b);
    /// less-then operator (time only)
    friend bool operator<(const AnimEvent& a, const AnimEvent& b);
    /// greather-then operator (time only)
    friend bool operator>(const AnimEvent& a, const AnimEvent& b);
    /// less-or-equal operator (time only)
    friend bool operator<=(const AnimEvent& a, const AnimEvent& b);
    /// greather-or-equal operator (time only)
    friend bool operator>=(const AnimEvent& a, const AnimEvent& b);

    Util::StringAtom name, category;
    Timing::Tick time;
};

//------------------------------------------------------------------------------
/**
*/
inline
AnimEvent::AnimEvent() :
    name(nullptr),
    category(nullptr),
    time(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
AnimEvent::AnimEvent(const Util::StringAtom& n, Timing::Tick t) :
    name(n),
    time(t)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
AnimEvent::AnimEvent(const Util::StringAtom& n, const Util::StringAtom& c, Timing::Tick t) :
    name(n),
    category(c),
    time(t)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator==(const AnimEvent& a, const AnimEvent& b)
{
    return (a.time == b.time);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator!=(const AnimEvent& a, const AnimEvent& b)
{
    return (a.time != b.time);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator<(const AnimEvent& a, const AnimEvent& b)
{
    return (a.time < b.time);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>(const AnimEvent& a, const AnimEvent& b)
{
    return (a.time > b.time);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator<=(const AnimEvent& a, const AnimEvent& b)
{
    return (a.time <= b.time);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>=(const AnimEvent& a, const AnimEvent& b)
{
    return (a.time >= b.time);
}

} // namespace CoreAnimation
//------------------------------------------------------------------------------
