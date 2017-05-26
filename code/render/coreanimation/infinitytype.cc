//------------------------------------------------------------------------------
//  coreanimation/infinitytype.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coreanimation/infinitytype.h"

namespace CoreAnimation
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
InfinityType::Code
InfinityType::FromString(const String& str)
{
    if (str == "Constant")       return Constant;
    else if (str == "Cycle")     return Cycle;
    else
    {
        return InvalidInfinityType;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
InfinityType::ToString(Code c)
{
    switch (c)
    {
        case Constant:  return "Constant";
        case Cycle:     return "Cycle";
        default:
            n_error("InfinityType::ToString(): invalid infinity type code!");
            return "";
    }
}

} // namespace CoreAnimation
