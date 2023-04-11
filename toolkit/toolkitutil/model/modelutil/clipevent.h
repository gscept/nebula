#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ClipEvent
    
    Implements a per-clip timed animation event. Simlar to animevent.h
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "timing/time.h"
namespace ToolkitUtil
{
class ClipEvent : public Core::RefCounted
{
    __DeclareClass(ClipEvent);
public:

    enum MarkerType
    {
        Ticks,
        Frames
    };

    /// constructor
    ClipEvent();
    /// destructor
    virtual ~ClipEvent();

    /// set the name of the event
    void SetName(const Util::String& name);
    /// get the name of the event
    const Util::String& GetName() const;

    /// set the name of the category
    void SetCategory(const Util::String& str);
    /// get the name of the category
    const Util::String& GetCategory() const;
    /// check if has category
    bool HasCategory() const;

    /// set the marker as time in milliseconds
    void SetMarkerAsMilliseconds(Timing::Tick time);
    /// set the marker as frame
    void SetMarkerAsFrames(int frame);

    /// returns the time, can be either marker or frame
    const int GetMarker() const;
    /// returns marker type
    const MarkerType GetMarkerType() const;

private:
    MarkerType markerType;
    Util::String name;
    Util::String category;
    Timing::Tick time;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
ClipEvent::SetName(const Util::String& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
ClipEvent::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ClipEvent::SetCategory(const Util::String& str)
{
    this->category = str;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
ClipEvent::GetCategory() const
{
    return this->category;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ClipEvent::HasCategory() const
{
    return this->category.IsValid();
}

//------------------------------------------------------------------------------
/**
*/
inline const int 
ClipEvent::GetMarker() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
inline const ClipEvent::MarkerType 
ClipEvent::GetMarkerType() const
{
    return this->markerType;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------