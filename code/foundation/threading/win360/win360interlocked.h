#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360Interlocked
    
    Provides simple atomic operations on shared variables.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include <windows.h>

//------------------------------------------------------------------------------
namespace Win360
{
class Win360Interlocked
{
public:
    /// interlocked increment, return result
    static long Increment(long volatile& var);
    /// interlocked decrement, return result
    static long Decrement(long volatile& var);
    /// interlocked add
    static long Add(long volatile& var, long add);
    /// interlocked exchange
    static long Exchange(long volatile* dest, long value);
	/// interlocked exchange
	static void* ExchangePointer(void* volatile* dest, void* value);
    /// interlocked compare-exchange
    static long CompareExchange(long volatile* dest, long exchange, long comparand);
	/// interlocked increment, return result
	static int Increment(int volatile& var);
	/// interlocked decrement, return result
	static int Decrement(int volatile& var);
	/// interlocked add
	static int Add(int volatile& var, int add);
	/// interlocked int
	static int Exchange(int volatile* dest, int value);
	/// interlocked compare-exchange
	static int CompareExchange(int volatile* dest, int exchange, int comparand);
};

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win360Interlocked::Increment(long volatile& var)
{
	return _InterlockedIncrement((volatile LONG*)&var);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win360Interlocked::Decrement(long volatile& var)
{
	return _InterlockedDecrement((volatile LONG*)&var);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win360Interlocked::Add(long volatile& var, long add)
{
	return _InterlockedExchangeAdd((volatile LONG*)&var, add);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win360Interlocked::Exchange(long volatile* dest, long value)
{
	return _InterlockedExchange((volatile LONG*)dest, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline long
Win360Interlocked::CompareExchange(long volatile* dest, long exchange, long comparand)
{
	return _InterlockedCompareExchange((volatile LONG*)dest, exchange, comparand);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win360Interlocked::Increment(int volatile& var)
{
    return _InterlockedIncrement((volatile LONG*)&var);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win360Interlocked::Decrement(int volatile& var)
{
    return _InterlockedDecrement((volatile LONG*)&var);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win360Interlocked::Add(int volatile& var, int add)
{
    return _InterlockedExchangeAdd((volatile LONG*)&var, add);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win360Interlocked::Exchange(int volatile* dest, int value)
{
    return _InterlockedExchange((volatile LONG*)dest, value);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline void*
Win360Interlocked::ExchangePointer(void* volatile* dest, void* value)
{
	return InterlockedExchangePointer(dest, value);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
Win360Interlocked::CompareExchange(int volatile* dest, int exchange, int comparand)
{
    return _InterlockedCompareExchange((volatile LONG*)dest, exchange, comparand);
}

} // namespace Win360
//------------------------------------------------------------------------------
