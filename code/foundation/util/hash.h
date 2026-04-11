#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::Hash 
  
    a generic hash function class based on murmurhash3.
      
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/

#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{

//------------------------------------------------------------------------------
/**
 * Hash based on murmurhash3
*/
uint32_t Hash(const uint8_t* key, SizeT len, uint32_t seed = 4711);


//------------------------------------------------------------------------------
/**
 * Hash combine function, based on boost::hash_combine
*/
inline uint32_t
HashCombineFast(uint32_t hash1, uint32_t hash2)
{
    return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

}
