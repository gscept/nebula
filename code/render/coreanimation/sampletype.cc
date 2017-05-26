//------------------------------------------------------------------------------
//  coreanimation/sampletype.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coreanimation/sampletype.h"

namespace CoreAnimation
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
SampleType::Code
SampleType::FromString(const String& str)
{
    if (str == "Step")            return Step;
    else if (str == "Linear")     return Linear;
    else if (str == "Hermite")    return Hermite;
    else if (str == "CatmullRom") return CatmullRom;
    else
    {
        return InvalidSampleType;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
SampleType::ToString(Code c)
{
    switch (c)
    {
        case Step:          return "Step";
        case Linear:        return "Linear";
        case Hermite:       return "Hermite";
        case CatmullRom:    return "CatmullRom";
        default:
            n_error("SampleType::ToString(): invalid sample type code!");
            return "";
    }
}

} // namespace CoreAnimation

