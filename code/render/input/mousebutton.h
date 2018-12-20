#pragma once
//------------------------------------------------------------------------------
/**
    @class Input::MouseButton
    
    Mouse button codes and conversion from/to string.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Input
{
class MouseButton
{
public:
    /// code enums
    enum Code
    {
        LeftButton   = 0,
        RightButton  = 1,
        MiddleButton = 2,

        NumMouseButtons,
        InvalidMouseButton,
    };
    
    /// convert to string
    static Util::String ToString(Code code);
    /// convert from string
    static Code FromString(const Util::String& str);
};

} // namespace Input
//------------------------------------------------------------------------------

    