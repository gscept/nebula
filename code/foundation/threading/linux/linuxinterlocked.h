#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::LinuxInterlocked

    Provides simple atomic operations on shared variables.

    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Linux
{
class LinuxInterlocked
{
public:
    /// interlocked increment
    static int Increment(int volatile& var);
    /// interlocked decrement
    static int Decrement(int volatile& var);
    /// interlocked add, return previous value
    static int Add(int volatile& var, int add);
    /// interlocked exchange
    static int Exchange(int volatile* dest, int value);
    /// interlocked exchange
    static void* ExchangePointer(void* volatile* dest, void* value);
    /// interlocked compare-exchange
    static int CompareExchange(int volatile* dest, int exchange, int comparand);
};

//------------------------------------------------------------------------------
/**
*/
inline int
LinuxInterlocked::Increment(int volatile& var)
{
    return __sync_add_and_fetch(&var, 1);
}

//------------------------------------------------------------------------------
/**
*/
inline int
LinuxInterlocked::Decrement(int volatile& var)
{
    return __sync_sub_and_fetch(&var, 1);
}

//------------------------------------------------------------------------------
/**
*/
inline int
LinuxInterlocked::Add(int volatile& var, int add)
{
    return __sync_fetch_and_add(&var, add);
}

//------------------------------------------------------------------------------
/**
*/
inline int
LinuxInterlocked::Exchange(int volatile* dest, int value)
{
    return __sync_lock_test_and_set(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
inline void*
LinuxInterlocked::ExchangePointer(void* volatile* dest, void* value)
{
    return __sync_lock_test_and_set(dest, value);
}

//------------------------------------------------------------------------------
/**
 */
inline int
LinuxInterlocked::CompareExchange(int volatile* dest, int exchange, int comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
}

} // namespace Linux
//------------------------------------------------------------------------------
