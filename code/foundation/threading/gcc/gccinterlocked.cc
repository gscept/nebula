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
Add(int volatile* var, int add)
{
    return __sync_fetch_and_add(var, add);
}

//------------------------------------------------------------------------------
/**
*/
int64
Add(int64 volatile* var, int64 add)
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
int64
Or(int64 volatile* var, int64 add)
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
int64
And(int64 volatile* var, int64 add)
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
int64
Xor(int64 volatile* var, int64 add)
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
int64
Exchange(int64 volatile* dest, int64 value)
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
int64
CompareExchange(int64 volatile* dest, int64 exchange, int64 comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
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
void*
CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand)
{
    return __sync_val_compare_and_swap(dest, comparand, exchange);
}

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
int64
Increment(int64 volatile* var)
{
    return __sync_add_and_fetch(var, 1);
}

//------------------------------------------------------------------------------
/**
*/
int64
Decrement(int64 volatile* var)
{
    return __sync_sub_and_fetch(var, 1);
}

} // namespace Interlocked

} // namespace Threading
