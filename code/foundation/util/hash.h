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
    Hash based on murmurhash64a.
*/
uint64_t Hash64(const uint8_t* key, SizeT len, uint64_t seed = 4711);

//------------------------------------------------------------------------------
/**
 * Hash combine function, based on boost::hash_combine
*/
inline uint32_t
HashCombineFast(uint32_t hash1, uint32_t hash2)
{
    return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

//------------------------------------------------------------------------------
/**
    Fold a 64 bit hash down to a 32 bit index. Taking the low half alone throws the high half away, so the
    two halves are combined before the fold
*/
inline uint32_t
HashCombineFast64(uint64_t hash)
{
    return HashCombineFast((uint32_t)(hash & 0xffffffff), (uint32_t)(hash >> 32));
}

}
