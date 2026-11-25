//------------------------------------------------------------------------------
//  hash.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "foundation/stdneb.h"
#include "util/hash.h"
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{
uint32_t 
Hash(const uint8_t* key, SizeT len, uint32_t seed)
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

//------------------------------------------------------------------------------
/**
*/
uint64_t
Hash64(const uint8_t* key, SizeT len, uint64_t seed)
{
    const static auto murmur64_scramble = [](uint64_t k)
    {
        k *= 0x87c379147876f98fULL;
        k = (k << 31) | (k >> 33);
        k *= 0x4cf5ad432775415fULL;
        return k;
    };

    uint64_t h = seed;
    const uint8_t* end = key + (len & ~7ULL);

    while (key < end)
    {
        uint64_t k;
        Memory::Copy(key, &k, 8);
        h ^= murmur64_scramble(k);
        h = (h << 27) | (h >> 37);
        h = h * 5 + 0x52dce729ULL;
        key += sizeof(uint64_t);
    }

    uint64_t k = 0;
    for (SizeT i = len & 7; i; i--)
    {
        k <<= 8;
        k |= *key++;
    }
    h ^= murmur64_scramble(k);
    h ^= (uint64_t)len;
    h ^= h >> 33;
    h *= 0x4cf5ad432775415fULL;
    h ^= h >> 29;
    h *= 0x87c379147876f98fULL;
    h ^= h >> 32;
    return h;
}

}
