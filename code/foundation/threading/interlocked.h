#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::Interlocked
    
    Provide simple atomic operations on memory variables.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "core/config.h"
namespace Threading
{

using int64 = int64_t;
typedef volatile int AtomicCounter;
typedef volatile int64 AtomicCounter64;

namespace Interlocked
{
using int64 = int64_t;

/// interlocked add
int Add(int volatile* var, int add);
/// interlocked add 64
int64 Add(int64 volatile* var, int64 add);
/// interlocked or
int Or(int volatile* var, int value);
/// interlocked or
int64 Or(int64 volatile* var, int64 value);
/// interlocked and
int And(int volatile* var, int value);
/// interlocked and
int64 And(int64 volatile* var, int64 value);
/// interlocked xor
int Xor(int volatile* var, int value);
/// interlocked xor
int64 Xor(int64 volatile* var, int64 value);
/// interlocked exchange
int Exchange(int volatile* dest, int value);
/// interlocked exchange
int64 Exchange(int64 volatile* dest, int64 value);
/// interlocked compare-exchange
int CompareExchange(int volatile* dest, int exchange, int comparand);
/// interlocked compare-exchange
int64 CompareExchange(int64 volatile* dest, int64 exchange, int64 comparand);
/// interlocked exchange
void* ExchangePointer(void* volatile* dest, void* value);
/// interlocked compare-exchange pointer
void* CompareExchangePointer(void* volatile* dest, void* exchange, void* comparand);
/// interlocked increment, return result
int Increment(int volatile* var);
/// interlocked increment, return result
int64 Increment(int64 volatile* var);
/// interlocked decrement, return result
int Decrement(int volatile* var);
/// interlocked decrement, return result
int64 Decrement(int64 volatile* var);

struct AtomicInt
{
    /// Add 
    int Add(int add) 
    {
        Threading::Interlocked::Add((volatile int*)&this->value, add);
    }
    /// Subtract
    int Sub(int sub)
    {
        Threading::Interlocked::Add((volatile int*)&this->value, -sub);
    }
    /// Or
    int Or(int mask)
    {
        Threading::Interlocked::Or((volatile int*)&this->value, mask);
    }
    /// And
    int And(int mask)
    {
        Threading::Interlocked::And((volatile int*)&this->value, mask);
    }
    /// Exchange
    int Exchange(int value)
    {
        Threading::Interlocked::Exchange((volatile int*)&this->value, value);
    }
    /// Compare and exchange
    int CompareExchange(int exchange, int comparand)
    {
        Threading::Interlocked::CompareExchange((volatile int*)&this->value, exchange, comparand);
    }
    /// Increment and return new value
    int Increment(int incr)
    {
        Threading::Interlocked::Increment((volatile int*)&this->value);
    }
    /// Decrement and return new value
    int Decrement(int decr)
    {
        Threading::Interlocked::Decrement((volatile int*)&this->value);
    }

private:
    volatile int value;
};

struct AtomicInt64
{
    /// Add 
    int64 Add(int64 add) 
    {
        Threading::Interlocked::Add((volatile int64*)&this->value, add);
    }
    /// Subtract
    int64 Sub(int64 sub)
    {
        Threading::Interlocked::Add((volatile int64*)&this->value, -sub);
    }
    /// Or
    int64 Or(int64 mask)
    {
        Threading::Interlocked::Or((volatile int64*)&this->value, mask);
    }
    /// And
    int64 And(int64 mask)
    {
        Threading::Interlocked::And((volatile int64*)&this->value, mask);
    }
    /// Exchange
    int64 Exchange(int64 value)
    {
        Threading::Interlocked::Exchange((volatile int64*)&this->value, value);
    }
    /// Compare and exchange
    int64 CompareExchange(int64 exchange, int64 comparand)
    {
        Threading::Interlocked::CompareExchange((volatile int64*)&this->value, exchange, comparand);
    }
    /// Increment and return new value
    int64 Increment(int64 incr)
    {
        Threading::Interlocked::Increment((volatile int64*)&this->value);
    }
    /// Decrement and return new value
    int64 Decrement(int64 decr)
    {
        Threading::Interlocked::Decrement((volatile int64*)&this->value);
    }
private:
    volatile int64 value;
};

struct AtomicPointer
{
    /// Exchange
    void* Exchange(void* value)
    {
        return Threading::Interlocked::ExchangePointer((void* volatile*)&this->ptr, value);
    }
    /// Compare and exchange
    void* CompareExchange(void* exchange, void* comparand)
    {
        return Threading::Interlocked::CompareExchangePointer((void* volatile*)&this->ptr, exchange, comparand);
    }
private:
    volatile void* ptr;
};


} // namespace Interlocked

} // namespace Threading
