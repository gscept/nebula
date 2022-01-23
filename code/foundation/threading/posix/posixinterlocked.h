#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixInterlocked
 
    Provides simple atomic operations on shared variables.
     
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixInterlocked
{
public:
    /// interlocked increment
    static int Increment(int volatile* var);
    /// interlocked decrement
    static int Decrement(int volatile* var);
    /// interlocked increment, return result
    static long Increment(long volatile* var);
    /// interlocked decrement, return result
    static long Decrement(long volatile* var);
    /// interlocked add
    static int Add(int volatile* var, int add);
    /// interlocked or
    static int Or(int volatile* var, int value);
    /// interlocked and
    static int And(int volatile* var, int value);
    /// interlocked xor
    static int Xor(int volatile* var, int value);
    /// interlocked exchange
    static int Exchange(int volatile* dest, int value);
    /// interlocked exchange
    static void* ExchangePointer(void* volatile* dest, void* value);
    /// interlocked compare-exchange
    static int CompareExchange(int volatile* dest, int exchange, int comparand);
    /// interlocked exchange
    static void* CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand);
};

//------------------------------------------------------------------------------
/**
*/
inline int
PosixInterlocked::Increment(int volatile* var)
{
    return __sync_add_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
inline int
PosixInterlocked::Decrement(int volatile* var)
{
    return __sync_sub_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
inline long
PosixInterlocked::Increment(long volatile* var)
{
    return __sync_add_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
inline long
PosixInterlocked::Decrement(long volatile* var)
{
    return __sync_sub_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
inline int
PosixInterlocked::Add(int volatile* var, int add)
{
    return __sync_fetch_and_add(var, add);
}

//------------------------------------------------------------------------------
/**
*/
inline int
PosixInterlocked::Or(int volatile* var, int add)
{
    return __sync_fetch_and_or(var, add);
}

//------------------------------------------------------------------------------
/**
*/
inline int
PosixInterlocked::And(int volatile* var, int add)
{
    return __sync_fetch_and_and(var, add);
}

//------------------------------------------------------------------------------
/**
*/
inline int
PosixInterlocked::Xor(int volatile* var, int add)
{
    return __sync_fetch_and_xor(var, add);
}

//------------------------------------------------------------------------------
/**
*/
inline int
PosixInterlocked::Exchange(int volatile* dest, int value)
{
    return __sync_lock_test_and_set(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
inline void*
PosixInterlocked::ExchangePointer(void* volatile* dest, void* value)
{
    return __sync_lock_test_and_set(dest, value);
}

//------------------------------------------------------------------------------
/**
 */
inline int
PosixInterlocked::CompareExchange(int volatile* dest, int exchange, int comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
}

//------------------------------------------------------------------------------
/**
*/
inline void*
PosixInterlocked::CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
}

} // namespace Posix
//------------------------------------------------------------------------------
