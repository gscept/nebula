#ifndef UTIL_RANDOMNUMBERTABLE_H
#define UTIL_RANDOMNUMNERTABLE_H
//------------------------------------------------------------------------------
/**
    @class Util::RandomNumberTable
    
    A table-based random-number generator. Returns the same random number
    for a given key.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
    
//------------------------------------------------------------------------------
namespace Util
{
class RandomNumberTable
{
public:
    /// return a pseudo-random number between 0 and 1
    static float Rand(IndexT key);
    /// return a pseudo random number between min and max
    static float Rand(IndexT key, float minVal, float maxVal);

private:
    static const SizeT tableSize = 2048;
    static const float randTable[tableSize];
};

//------------------------------------------------------------------------------
/**
*/
inline float
RandomNumberTable::Rand(IndexT key)
{
    return randTable[key % tableSize];
}

//------------------------------------------------------------------------------
/**
*/
inline float
RandomNumberTable::Rand(IndexT key, float minVal, float maxVal)
{
    return minVal + (randTable[key % tableSize] * (maxVal - minVal));
}

} // namespace Util
//------------------------------------------------------------------------------
#endif
