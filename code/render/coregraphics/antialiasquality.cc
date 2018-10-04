//------------------------------------------------------------------------------
//  antialiasquality.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/antialiasquality.h"

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
AntiAliasQuality::Code
AntiAliasQuality::FromString(const Util::String& str)
{
    if ("None" == str) return None;
    else if ("Low" == str) return Low;
    else if ("Medium" == str) return Medium;
    else if ("High" == str) return High;
    else
    {
        n_error("Invalid antialias quality string '%s'!", str.AsCharPtr());
        return None;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::String
AntiAliasQuality::ToString(Code code)
{
    switch (code)
    {
        case None:      return "None";
        case Low:       return "Low";
        case Medium:    return "Medium";
        case High:      return "High";
        default:
            n_error("Invalid antialias quality code!");
            return "";
    }
}

} // namespace CoreGraphics
