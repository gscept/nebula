//------------------------------------------------------------------------------
//  adapter.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/adapter.h"

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
Adapter::Code
Adapter::FromString(const Util::String& str)
{
    if ("None" == str) return None;
    else if ("Primary" == str) return Primary;
    else if ("Secondary" == str) return Secondary;
    else
    {
        n_error("Invalid adapter string '%s'!", str.AsCharPtr());
        return Primary;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::String
Adapter::ToString(Code code)
{
    switch (code)
    {
        case None:      return "None";
        case Primary:   return "Primary";
        case Secondary: return "Secondary";
        default:
            n_error("Invalid adapter code!");
            return "";
    }
}

} // namespace CoreGraphics
