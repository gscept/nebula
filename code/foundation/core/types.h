#pragma once
//------------------------------------------------------------------------------
/**
    @class core/types.h

    Basic type definitions for Nebula.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/

#if !SPU
#include "memory/memory.h"
#endif

#include <stddef.h>
#include <stdint.h>
// fixing Windows defines...
#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef CopyFile
#undef CopyFile
#endif
#ifdef GetObject
#undef GetObject
#endif

typedef unsigned long  ulong;
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef unsigned char  ubyte;

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
typedef int8_t int8;

typedef uintptr_t uintptr;
typedef ptrdiff_t ptrdiff;

typedef int IndexT;			// the index type
typedef int SizeT;			// the size type
typedef uintptr PtrT;		// the ptr type
typedef ptrdiff PtrDiff;
static const int InvalidIndex = -1;

//------------------------------------------------------------------------------
/**
*/
constexpr uint64
operator"" _KB(const uint64 val)
{
	return val * 1024;
}

//------------------------------------------------------------------------------
/**
*/
constexpr uint64
operator"" _MB(const uint64 val)
{
	return val * 1024 * 1024;
}

#define N_BIT(x) (1 << x)

#define N_ARGB(a,r,g,b) ((uint)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define N_RGBA(r,g,b,a) N_ARGB(a,r,g,b)
#define N_XRGB(r,g,b)   N_ARGB(0xff,r,g,b)
#define N_COLORVALUE(r,g,b,a) N_RGBA((uint)((r)*255.f),(uint)((g)*255.f),(uint)((b)*255.f),(uint)((a)*255.f))

template <class FLAGS, class BITS>
constexpr bool
CheckBits(const FLAGS flags, const BITS bits)
{
    return (flags & bits) == bits;
}

// byte bit calc
#define BITS_TO_BYTES(x) (((x)+7)>>3)
#define BYTES_TO_BITS(x) ((x)<<3)

#if (__OSX__ || __linux__)
inline ushort                _byteswap_ushort(ushort x)              { return ((x>>8) | (x<<8)); }
inline ulong                 _byteswap_ulong(ulong x)                { return ((x&0xff000000)>>24) | ((x&0x00ff0000)>>8) | ((x&0x00000ff00)<<8) | ((x&0x000000ff)<<24); }
inline unsigned long long    _byteswap_uint64(unsigned long long x)  { return ((((unsigned long long)_byteswap_ulong((ulong)(x & 0xffffffff))) << 32) | ((unsigned long long)_byteswap_ulong((ulong)(x >> 32)))); }
#endif

#if __linux__
typedef unsigned char byte;
#endif
#if __WIN32__
#define n_stricmp stricmp
#define n_snprintf StringCchPrintf
#elif (__OSX__ || __APPLE__ || __linux__ )
#define n_stricmp strcasecmp
#define n_snprintf sprintf
#else
#error "Unsupported platform!"
#endif

#if __MAYA__
#define ThreadLocal
#elif __WIN32__
#define ThreadLocal __declspec(thread)
#elif __linux__
#define ThreadLocal __thread
#if (__OSX__ || __APPLE__)
// thread locals are not allowed on osx, so we define thread local as nothing to prevent problems
#undef ThreadLocal
#define ThreadLocal
#endif
#else
#error "Unsupported platform!"
#endif

#define NEBULA_ALIGN16 alignas(16)
//------------------------------------------------------------------------------
