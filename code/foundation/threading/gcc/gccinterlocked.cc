#pragma once
//------------------------------------------------------------------------------
/**
    @class Gcc::GccInterlocked
 
    Provides simple atomic operations on shared variables using
    gcc compiler builtins
     
    (C) 2013-2023 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace Threading
{

namespace Interlocked
{

//------------------------------------------------------------------------------
/**
*/
int
Increment(int volatile* var)
{
    return __sync_add_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
int
Decrement(int volatile* var)
{
    return __sync_sub_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
long
Increment(int64 volatile* var)
{
    return __sync_add_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
long
Decrement(int64 volatile* var)
{
    return __sync_sub_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
int
Add(int volatile* var, int add)
{
    return __sync_fetch_and_add(var, add);
}

//------------------------------------------------------------------------------
/**
*/
int
Or(int volatile* var, int add)
{
    return __sync_fetch_and_or(var, add);
}

//------------------------------------------------------------------------------
/**
*/
int
And(int volatile* var, int add)
{
    return __sync_fetch_and_and(var, add);
}

//------------------------------------------------------------------------------
/**
*/
int
Xor(int volatile* var, int add)
{
    return __sync_fetch_and_xor(var, add);
}

//------------------------------------------------------------------------------
/**
*/
int
Exchange(int volatile* dest, int value)
{
    return __sync_lock_test_and_set(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
void*
ExchangePointer(void* volatile* dest, void* value)
{
    return __sync_lock_test_and_set(dest, value);
}

//------------------------------------------------------------------------------
/**
 */
int
CompareExchange(int volatile* dest, int exchange, int comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
}

//------------------------------------------------------------------------------
/**
*/
void*
CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
}

} // namespace Interlocked

} // namespace Threading
