//------------------------------------------------------------------------------
//  mousebutton.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "input/mousebutton.h"

namespace Input
{

//------------------------------------------------------------------------------
/**
*/
Util::String
MouseButton::ToString(Code code)
{
    switch (code)
    {
        case LeftButton:    return "LeftButton";
        case RightButton:   return "RightButton";
        case MiddleButton:  return "MiddleButton";
		default:			break;
				
    }
    n_error("Invalid mouse button code!\n");
    return "";
}

//------------------------------------------------------------------------------
/**
*/
MouseButton::Code
MouseButton::FromString(const Util::String& str)
{
    if ("LeftButton" == str) return LeftButton;
    else if ("RightButton" == str) return RightButton;
    else if ("MiddleButton" == str) return MiddleButton;
    else
    {
        n_error("Invalid mouse button string!\n");
        return InvalidMouseButton;
    }
}

} // namespace Input