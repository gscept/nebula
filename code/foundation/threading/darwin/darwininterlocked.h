#pragma once
#ifndef DARWIN_DARWININTERLOCKED_H
#define DARWIN_DARWININTERLOCKED_H
//------------------------------------------------------------------------------
/**
    @class Darwin::DarwinInterlocked
    
    Provides simple atomic operations on shared variables.
    
    (C) 2008 Bruce Mitchener, Jr.
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include <libkern/OSAtomic.h>

//------------------------------------------------------------------------------
namespace Darwin
{
class DarwinInterlocked
{
public:
    /// interlocked increment
    static int Increment(int volatile& var);
    /// interlocked decrement
    static int Decrement(int volatile& var);
    /// interlocked add
    static void Add(int volatile& var, int add);
};

//------------------------------------------------------------------------------
/**
*/
inline int
DarwinInterlocked::Increment(int volatile& var)
{
    return OSAtomicIncrement32((volatile int32_t*)&var);
}

//------------------------------------------------------------------------------
/**
*/
inline int
DarwinInterlocked::Decrement(int volatile& var)
{
    return OSAtomicDecrement32((volatile int32_t*)&var);
}

//------------------------------------------------------------------------------
/**
*/
inline void
DarwinInterlocked::Add(int volatile& var, int add)
{
    OSAtomicAdd32(add, (volatile int32_t*)&var);
}

} // namespace Darwin
//------------------------------------------------------------------------------
#endif
