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
uint32_t Hash(const uint8_t* key, SizeT len, uint32_t seed = 4711)
{

    const static auto murmur_scramble = [](uint32_t k)
    {
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        return k;
    };

    uint32_t h = seed;
    uint32_t k;

    for (size_t i = len >> 2; i; i--) 
    {
        Memory::Copy(key, &k, 4);
        key += sizeof(uint32_t);
        h ^= murmur_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    k = 0;
    for (size_t i = len & 3; i; i--) 
    {
        k <<= 8;
        k |= *key++;
    }
    h ^= murmur_scramble(k);
    h ^= (uint32_t)len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

}
