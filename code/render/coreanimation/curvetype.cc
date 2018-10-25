//------------------------------------------------------------------------------
//  coreanimation/curvetype.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/curvetype.h"

namespace CoreAnimation
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
CurveType::Code
CurveType::FromString(const String& str)
{
    if (str == "Translation") return Translation;
    else if (str == "Scale") return Scale;
    else if (str == "Rotation") return Rotation;
    else if (str == "Color") return Color;
    else if (str == "Velocity") return Velocity;
    else if (str == "Float4") return Float4;
    else
    {
        return InvalidCurveType;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
CurveType::ToString(Code c)
{
    switch (c)
    {
        case Translation:   return "Translation";
        case Scale:         return "Scale";
        case Rotation:      return "Rotation";
        case Color:         return "Color";
        case Velocity:      return "Velocity";
        case Float4:        return "Float4";
        default:
            n_error("CurveType::ToString(): invalid curve type code!");
            return "";
    }
}

} // namespace CoreAnimation