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

//------------------------------------------------------------------------------
namespace Win32
{
class Win32Interlocked
{
public:
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
    /// interlocked compare-exchange
    static int CompareExchange(int volatile* dest, int exchange, int comparand);
    /// interlocked add
    static long Add(long volatile* var, long add);
    /// interlocked or
    static long Or(long volatile* var, long value);
    /// interlocked and
    static long And(long volatile* var, long value);
    /// interlocked xor
    static long Xor(long volatile* var, long value);
    /// interlocked exchange
    static long Exchange(long volatile* dest, long value);
    /// interlocked compare-exchange
    static long CompareExchange(long volatile* dest, long exchange, long comparand);
    /// interlocked exchange of two int32 or single 64 bit (pointer or integer) values simultaneously
    static bool CompareExchange128(int64_t volatile* dest, int64_t* exchange, int64_t* comparand);
    /// interlocked exchange
    static void* ExchangePointer(void* volatile* dest, void* value);
    /// interlocked compare-exchange pointer
    static void* CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand);
    /// interlocked increment, return result
    static int Increment(int volatile* var);
    /// interlocked decrement, return result
    static int Decrement(int volatile* var);
    /// interlocked increment, return result
    static long Increment(long volatile* var);
    /// interlocked decrement, return result
    static long Decrement(long volatile* var);

};

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win32Interlocked::Add(int volatile* var, int add)
{
    return _InterlockedExchangeAdd((volatile LONG*)var, add);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int 
Win32Interlocked::Or(int volatile* var, int value)
{
    return _InterlockedOr((volatile LONG*)var, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int 
Win32Interlocked::And(int volatile* var,int value)
{
    return _InterlockedAnd((volatile LONG*)var, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int 
Win32Interlocked::Xor(int volatile* var,int value)
{
    return _InterlockedXor((volatile LONG*)var, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win32Interlocked::Exchange(int volatile* dest, int value)
{
    return _InterlockedExchange((volatile LONG*)dest, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win32Interlocked::CompareExchange(int volatile* dest, int exchange, int comparand)
{
    return _InterlockedCompareExchange((volatile LONG*)dest, exchange, comparand);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win32Interlocked::Add(long volatile* var, long add)
{
    return _InterlockedExchangeAdd(var, add);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long 
Win32Interlocked::Or(long volatile* var, long value)
{
    return _InterlockedOr(var, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long 
Win32Interlocked::And(long volatile* var, long value)
{
    return _InterlockedAnd(var, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long 
Win32Interlocked::Xor(long volatile* var, long value)
{
    return _InterlockedXor(var, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win32Interlocked::Exchange(long volatile* dest, long value)
{
    return _InterlockedExchange(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win32Interlocked::CompareExchange(long volatile* dest, long exchange, long comparand)
{
    return _InterlockedCompareExchange(dest, exchange, comparand);
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
Win32Interlocked::CompareExchange128(int64_t volatile* dest, int64_t* exchange, int64_t* comparand)
{
    return _InterlockedCompareExchange128(dest, exchange[0], exchange[1], comparand);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void*
Win32Interlocked::ExchangePointer(void* volatile* dest, void* value)
{
    return InterlockedExchangePointer(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void* 
Win32Interlocked::CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand)
{
    return _InterlockedCompareExchangePointer(dest, exchange, comparand);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win32Interlocked::Increment(int volatile* var)
{
    return _InterlockedIncrement((volatile LONG*)var);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win32Interlocked::Decrement(int volatile* var)
{
    return _InterlockedDecrement((volatile LONG*)var);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win32Interlocked::Increment(long volatile* var)
{
    return _InterlockedIncrement(var);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win32Interlocked::Decrement(long volatile* var)
{
    return _InterlockedDecrement(var);
}

} // namespace Win32
//------------------------------------------------------------------------------
