#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::OSXInterlocked
 
    Provides simple atomic operations on shared variables.
 
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace OSX
{
class OSXInterlocked
{
public:
    /// interlocked increment
    static int Increment(int volatile& var);
    /// interlocked decrement
    static int Decrement(int volatile& var);
    /// interlocked add
    static void Add(int volatile& var, int add);
    /// interlocked exchange
    static int Exchange(int volatile* dest, int value);
    /// interlocked compare-exchange
    static int CompareExchange(int volatile* dest, int exchange, int comparand);
};

//------------------------------------------------------------------------------
/**
*/
inline int
OSXInterlocked::Increment(int volatile& var)
{
    return __sync_add_and_fetch(&var, 1);
}
    
//------------------------------------------------------------------------------
/**
*/
inline int
OSXInterlocked::Decrement(int volatile& var)
{
    return __sync_sub_and_fetch(&var, 1);
}
    
//------------------------------------------------------------------------------
/**
*/
inline void
OSXInterlocked::Add(int volatile& var, int add)
{
    __sync_add_and_fetch(&var, add);
}
   
//------------------------------------------------------------------------------
/**
*/
inline int
OSXInterlocked::Exchange(int volatile* dest, int value)
{
    return __sync_lock_test_and_set(dest, value);
}
    
//------------------------------------------------------------------------------
/**
 */
inline int
OSXInterlocked::CompareExchange(int volatile* dest, int exchange, int comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
}

} // namespace OSX
//------------------------------------------------------------------------------
