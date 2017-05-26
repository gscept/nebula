#pragma once
//------------------------------------------------------------------------------
/**
    @class Animator::AnimLoopType

    Legacy N2 stuff!

    (C) 2008 RadonLabs GmbH
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Models
{
class AnimLoopType
{
public:
    enum Type
    {
        Loop,
        Clamp
    };

    // convert to string
    static Util::String ToString(AnimLoopType::Type t);
    // convert from string
    static AnimLoopType::Type FromString(const Util::String& s);
};

//------------------------------------------------------------------------------
/**
*/
inline
Util::String
AnimLoopType::ToString(AnimLoopType::Type t)
{
    switch (t)
    {
        case Loop:  
        {
            return Util::String("loop");
        }
        case Clamp: 
        {
            return Util::String("clamp");
        }
        default:    
        {
            return Util::String("clamp");
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
AnimLoopType::Type
AnimLoopType::FromString(const Util::String& s)
{
    if (s == "loop") 
    {
        return Loop;
    }
    else
    {
        return Clamp;
    }
}

} // namespace Models
