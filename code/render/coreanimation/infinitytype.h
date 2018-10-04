#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::InfinityType
  
    Describes how time position outsides of an animation curve's scope 
    are handled.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#if !__NEBULA_JOB__
#include "util/string.h"
#endif

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class InfinityType
{
public:
    /// anim infinity types
    enum Code
    {
        Constant,       //> return first or last key
        Cycle,          //> loop the animation
    
        NumInfinityTypes,
        InvalidInfinityType,
    };
    
    #if !__NEBULA_JOB__
    /// convert from string
    static Code FromString(const Util::String& str);
    /// convert to string
    static Util::String ToString(Code c);
    #endif
};

} // namespace CoreAnimation
//------------------------------------------------------------------------------
