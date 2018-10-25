#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Adapter
    
    Display adapter enum.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class Adapter
{
public:
    /// enum
    enum Code
    {
        Primary = 0,
        Secondary,    
        None,
    };

    /// convert adapter code from string
    static Code FromString(const Util::String& str);
    /// convert adapter code to string
    static Util::String ToString(Code code);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------
