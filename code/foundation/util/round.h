#pragma once
//------------------------------------------------------------------------------
/**
    @class Round
    
    Helper class for rounding up integer values to 2^N values.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
class Round
{
public:
    /// round up to nearest 2 bytes boundary
    static uint RoundUp2(uint val);
    /// round up to nearest 4 bytes boundary
    static uint RoundUp4(uint val);
    /// round up to nearest 8 bytes boundary
    static uint RoundUp8(uint val);
    /// round up to nearest 16 bytes boundary
    static uint RoundUp16(uint val);
    /// round up to nearest 32 bytes boundary
    static uint RoundUp32(uint val);    
    /// generic roundup
    static uint RoundUp(uint val, uint boundary);
};

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
Round::RoundUp2(uint val)
{
    return ((uint)val + (2 - 1)) & ~(2 - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
Round::RoundUp4(uint val)
{
    return ((uint)val + (4 - 1)) & ~(4 - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
Round::RoundUp8(uint val)
{
    return ((uint)val + (8 - 1)) & ~(8 - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
Round::RoundUp16(uint val)
{
    return ((uint)val + (16 - 1)) & ~(16 - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
Round::RoundUp32(uint val)
{
    return ((uint)val + (32 - 1)) & ~(32 - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
Round::RoundUp(uint val, uint boundary)
{
    n_assert(0 != boundary);
    return (((val - 1) / boundary) + 1) * boundary;
}

} // namespace Util
//------------------------------------------------------------------------------
