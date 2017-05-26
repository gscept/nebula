//------------------------------------------------------------------------------
//  lighttype.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lighting/lighttype.h"

namespace Lighting
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
LightType::Code
LightType::FromString(const String& str)
{
    if (str == "Global") return Global;
    else if (str == "Spot") return Spot;
    else if (str == "Point") return Point;
    else
    {
        n_error("Invalid light type string: %s", str.AsCharPtr());
        return InvalidLightType;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
LightType::ToString(Code code)
{
    switch (code)
    {
        case Global:    return "Global";
        case Spot:      return "Spot";
        case Point:     return "Point";
        default:
        n_error("Invalid LightType code!");
    }
    return "";
}

} // namespace Lighting