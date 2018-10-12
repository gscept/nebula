#pragma once
//------------------------------------------------------------------------------
/**
    @class Animation::AnimEventInfo

    The AnimEventInfo has extra information of the animevent, 
    like the animjob from which it is initiated
      
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "coreanimation/animevent.h"
#include "timing/time.h"
#include "util/stringatom.h"
#include "graphics/graphicsentity.h"

//------------------------------------------------------------------------------
namespace Animation
{
class AnimEventInfo
{
public:
    /// constructor
    AnimEventInfo();

    /// equality operator (time only)
    friend bool operator==(const AnimEventInfo& a, const AnimEventInfo& b);
    /// inequality operator (time only)
    friend bool operator!=(const AnimEventInfo& a, const AnimEventInfo& b);
    /// less-then operator (time only)
    friend bool operator<(const AnimEventInfo& a, const AnimEventInfo& b);
    /// greather-then operator (time only)
    friend bool operator>(const AnimEventInfo& a, const AnimEventInfo& b);
    /// less-or-equal operator (time only)
    friend bool operator<=(const AnimEventInfo& a, const AnimEventInfo& b);
    /// greather-or-equal operator (time only)
    friend bool operator>=(const AnimEventInfo& a, const AnimEventInfo& b);
    
    /// get AnimEvent	
    const CoreAnimation::AnimEvent& GetAnimEvent() const;
    /// set AnimEvent
    void SetAnimEvent(const CoreAnimation::AnimEvent& val);
    
    /// get ClipName	
    const Util::StringAtom& GetAnimJobName() const;
    /// set ClipName
    void SetAnimJobName(const Util::StringAtom& val);
    
    /// get Weight	
    float GetWeight() const;
    /// set Weight
    void SetWeight(float val);
            
    /// set id
    void SetEntityId(const Graphics::GraphicsEntity::Id& id);
    /// get id 
    const Graphics::GraphicsEntity::Id& GetEntityId() const;

private:
    CoreAnimation::AnimEvent animEvent;
    Util::StringAtom animJobName;
    float weight;
    Graphics::GraphicsEntity::Id id;
};

//------------------------------------------------------------------------------
/**
*/
inline
AnimEventInfo::AnimEventInfo(): weight(0.0f),
                                id(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator==(const AnimEventInfo& a, const AnimEventInfo& b)
{
    return (a.animEvent == b.animEvent);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator!=(const AnimEventInfo& a, const AnimEventInfo& b)
{
    return (a.animEvent != b.animEvent);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator<(const AnimEventInfo& a, const AnimEventInfo& b)
{
    return (a.animEvent < b.animEvent);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>(const AnimEventInfo& a, const AnimEventInfo& b)
{
    return (a.animEvent > b.animEvent);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator<=(const AnimEventInfo& a, const AnimEventInfo& b)
{
    return (a.animEvent <= b.animEvent);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>=(const AnimEventInfo& a, const AnimEventInfo& b)
{
    return (a.animEvent >= b.animEvent);
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreAnimation::AnimEvent& 
AnimEventInfo::GetAnimEvent() const
{
    return this->animEvent;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AnimEventInfo::SetAnimEvent(const CoreAnimation::AnimEvent& val)
{
    this->animEvent = val;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom& 
AnimEventInfo::GetAnimJobName() const
{
    return this->animJobName;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AnimEventInfo::SetAnimJobName(const Util::StringAtom& val)
{
    this->animJobName = val;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
AnimEventInfo::GetWeight() const
{
    return this->weight;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AnimEventInfo::SetWeight(float val)
{
    this->weight = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
AnimEventInfo::SetEntityId(const Graphics::GraphicsEntity::Id& id)
{
    this->id = id;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Graphics::GraphicsEntity::Id&
AnimEventInfo::GetEntityId() const
{
    return this->id;
}
} // namespace Animation
//------------------------------------------------------------------------------