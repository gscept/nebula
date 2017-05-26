#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::LightType
    
    Identifies different light types.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Lighting
{
class LightType
{
public:
    enum Code
    {
        Global = 0,
        Spot,
        Point,

        NumLightTypes,
        InvalidLightType = 0xffffffff,      // force size to 32 bit
    };
    /// convert from string
    static Code FromString(const Util::String& str);
    /// convert to string
    static Util::String ToString(Code code);
}; 

}   // namespace Lighting
//------------------------------------------------------------------------------
    