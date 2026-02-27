#pragma once
//------------------------------------------------------------------------------
/**
    @file bit.h
    
    Implements helper functions for checking bits
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace Util
{

//------------------------------------------------------------------------------
/**
*/
constexpr uint64_t 
SetBit(uint64_t mask, uint8_t bit)
{
    return mask | (1ULL << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(uint64_t mask, uint8_t bit)
{
    return mask & (1ULL << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint32_t
SetBit(uint32_t mask, uint8_t bit)
{
    return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(uint32_t mask, uint8_t bit)
{
    return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint16_t
SetBit(uint16_t mask, uint8_t bit)
{
    return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(uint16_t mask, uint8_t bit)
{
    return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr int64_t
SetBit(int64_t mask, uint8_t bit)
{
    return mask | (1LL << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(int64_t mask, uint8_t bit)
{
    return mask & (1LL << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr int32_t
SetBit(int32_t mask, uint8_t bit)
{
    return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(int32_t mask, uint8_t bit)
{
    return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr int16_t
SetBit(int16_t mask, uint8_t bit)
{
    return mask | (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr bool
HasBit(int16_t mask, uint8_t bit)
{
    return mask & (1 << bit);
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint32_t
CountBits(uint32_t i)
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

//------------------------------------------------------------------------------
/**
    Combine hashes
*/
template<typename T>
inline void HashCombine(uint32_t& s, const T& v)
{
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

//------------------------------------------------------------------------------
/**
*/
inline uint
PopCnt(uint value)
{
	return _mm_popcnt_u32(value);
}

//------------------------------------------------------------------------------
/**
*/
inline uint64_t
PopCnt(uint64_t value)
{
	return _mm_popcnt_u64(value);
}

//------------------------------------------------------------------------------
/**
*/
inline uint
FirstBitSetIndex(uint value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanForward(&count, value);
#else
    int count = __builtin_ctz(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
FirstBitSetIndex(uint64_t value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanForward64(&count, value);
#else
    int count = __builtin_ctzll(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
LastBitSetIndex(uint value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanReverse(&count, value);
#else
    int count = 31 - __builtin_clz(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
LastBitSetIndex(uint64_t value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanReverse64(&count, value);
#else
    int count = 63 - __builtin_clzll(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
CountLeadingZeroes(uint value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanReverse(&count, value);
    count = 31 - count;
#else
    int count = __builtin_clz(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
CountLeadingZeroes(uint64_t value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanReverse64(&count, value);
    count = 31 - count;
#else
    int count = __builtin_clzll(value);
#endif
    return count;
} 

//------------------------------------------------------------------------------
/**
*/
inline uint
CountTrailingZeroes(uint value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanForward(&count, value);
#else
    int count = __builtin_ctz(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
CountTrailingZeroes(uint64_t value)
{
#if __WIN32__
    DWORD count = 0;
    _BitScanForward64(&count, value);
    count = count;
#else
    int count = __builtin_ctzll(value);
#endif
    return count;
}

//------------------------------------------------------------------------------
/**
*/
inline uint
Lsb(uint value, byte bit)
{
    uint mask = value & ~((1 << bit) - 1);
#if __WIN32__
    DWORD count = 0;
    _BitScanForward(&count, mask);
#else
    int count = __builtin_ctz(mask);
#endif
    return mask ? count : 0xFFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
inline uint 
BitmaskConvert(uint mask, const uint* table, const uint numEntries = 0xFFFFFFFF)
{
    uint ret = 0x0;
    uint usageFlags = mask;
    uint flagIndex = 0;
    while (usageFlags != 0x0 && flagIndex < numEntries)
    {
        if (usageFlags & (1 << flagIndex))
        {
            ret |= table[flagIndex];
        }
        usageFlags &= ~(1 << flagIndex);
        flagIndex++;
    }
    return ret;
}

} // namespace Util

