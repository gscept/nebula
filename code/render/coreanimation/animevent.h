#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimEvent
  
    An animation event associates a name with a point in time. The event
    will be triggered when the play-cursor passes the point in time of the
    event. AnimEvents are attached to anim clips.
    
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
            
    /// set the name of the event
    void SetName(const Util::StringAtom& name);
    /// get the name of the event
    const Util::StringAtom& GetName() const;

    /// set the name of the category
    void SetCategory(const Util::StringAtom& str);
    /// get the name of the category
    const Util::StringAtom& GetCategory() const;
    /// check if has category
    bool HasCategory() const;
    
    /// set the point-in-time when the event should trigger in seconds
    void SetTime(Timing::Tick t);
    /// get the time when the event should trigger
    Timing::Tick GetTime() const;

private:
    Util::StringAtom name;
    Util::StringAtom category;
    Timing::Tick time;
};

//------------------------------------------------------------------------------
/**
*/
inline
AnimEvent::AnimEvent() :
    time(0),
	name(nullptr),
	category(nullptr)
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
inline void
AnimEvent::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
AnimEvent::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimEvent::SetCategory(const Util::StringAtom& str)
{
    this->category = str;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
AnimEvent::GetCategory() const
{
    return this->category;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimEvent::HasCategory() const
{
    return this->category.IsValid();
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimEvent::SetTime(Timing::Tick t)
{
    this->time = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimEvent::GetTime() const
{
    return this->time;
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