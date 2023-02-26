#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::CurveType
  
    Describes the general data type of the keys stored in an animation curve.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#if !__NEBULA_JOB__
#include "util/string.h"
#endif

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class CurveType
{
public:
    /// animation curve types, keep this order!
    enum Code
    {
        Translation,    //> keys in curve describe a translation
        Rotation,       //> keys in curve describe a rotation quaternion
        Scale,          //> keys in curve describe a scale
        Color,          //> keys in curve describe a color
        Velocity,       //> keys describe a linear velocition
        Float4,         //> generic 4D key

        NumCurveTypes,
        InvalidCurveType,
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

