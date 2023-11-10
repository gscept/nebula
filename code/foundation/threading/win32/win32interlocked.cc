#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Interlocked
    
    Provides simple atomic operations on shared variables.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
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
    return _InterlockedExchangeAdd((volatile LONG*)var, add);
}

//------------------------------------------------------------------------------
/**
*/
int
Or(int volatile* var, int value)
{
    return _InterlockedOr((volatile LONG*)var, value);
}

//------------------------------------------------------------------------------
/**
*/
int
And(int volatile* var, int value)
{
    return _InterlockedAnd((volatile LONG*)var, value);
}

//------------------------------------------------------------------------------
/**
*/
int
Xor(int volatile* var, int value)
{
    return _InterlockedXor((volatile LONG*)var, value);
}

//------------------------------------------------------------------------------
/**
*/
int
Exchange(int volatile* dest, int value)
{
    return _InterlockedExchange((volatile LONG*)dest, value);
}

//------------------------------------------------------------------------------
/**
*/
int
CompareExchange(int volatile* dest, int exchange, int comparand)
{
    return _InterlockedCompareExchange((volatile LONG*)dest, exchange, comparand);
}

//------------------------------------------------------------------------------
/**
*/
int64
Add(int64 volatile* var, int64 add)
{
    return _InterlockedExchangeAdd64(var, add);
}

//------------------------------------------------------------------------------
/**
*/
int64
Or(int64 volatile* var, int64 value)
{
    return _InterlockedOr64(var, value);
}

//------------------------------------------------------------------------------
/**
*/
int64
And(int64 volatile* var, int64 value)
{
    return _InterlockedAnd64(var, value);
}

//------------------------------------------------------------------------------
/**
*/
int64
Xor(int64 volatile* var, int64 value)
{
    return _InterlockedXor64(var, value);
}

//------------------------------------------------------------------------------
/**
*/
int64
Exchange(int64 volatile* dest, int64 value)
{
    return _InterlockedExchange64(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
int64
CompareExchange(int64 volatile* dest, int64 exchange, int64 comparand)
{
    return _InterlockedCompareExchange64(dest, exchange, comparand);
}

//------------------------------------------------------------------------------
/**
*/
void*
ExchangePointer(void* volatile* dest, void* value)
{
    return InterlockedExchangePointer(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
void*
CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand)
{
    return _InterlockedCompareExchangePointer(dest, exchange, comparand);
}

//------------------------------------------------------------------------------
/**
*/
int
Increment(int volatile* var)
{
    return _InterlockedIncrement((volatile LONG*)var);
}

//------------------------------------------------------------------------------
/**
*/
int
Decrement(int volatile* var)
{
    return _InterlockedDecrement((volatile LONG*)var);
}

//------------------------------------------------------------------------------
/**
*/
int64
Increment(int64 volatile* var)
{
    return _InterlockedIncrement64(var);
}

//------------------------------------------------------------------------------
/**
*/
int64
Decrement(int64 volatile* var)
{
    return _InterlockedDecrement64(var);
}

} // namespace Interlocked

} // namespace Threading
