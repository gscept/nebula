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
#include <new>
namespace Threading
{

using int64 = int64_t;

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

struct alignas(std::hardware_destructive_interference_size) AtomicInt
{
    /// Constructor
    AtomicInt()
        : value(0)
    {}

    /// Constructor
    AtomicInt(int initial)
        : value(initial)
    {}

    /// Assignment (unsafe)
    void operator=(int value)
    {
        this->value = value;
    }

    /// Add 
    int Add(int add) 
    {
        return Threading::Interlocked::Add((volatile int*)&this->value, add);
    }
    /// Subtract
    int Sub(int sub)
    {
        return Threading::Interlocked::Add((volatile int*)&this->value, -sub);
    }
    /// Or
    int Or(int mask)
    {
        return Threading::Interlocked::Or((volatile int*)&this->value, mask);
    }
    /// And
    int And(int mask)
    {
        return Threading::Interlocked::And((volatile int*)&this->value, mask);
    }
    /// Exchange
    int Exchange(int value)
    {
        return Threading::Interlocked::Exchange((volatile int*)&this->value, value);
    }
    /// Compare and exchange
    int CompareExchange(int exchange, int comparand)
    {
        return Threading::Interlocked::CompareExchange((volatile int*)&this->value, exchange, comparand);
    }
    /// Increment and return new value
    int Increment(int incr)
    {
        return Threading::Interlocked::Add((volatile int*)&this->value, incr);
    }
    /// Decrement and return new value
    int Decrement(int decr)
    {
        return Threading::Interlocked::Add((volatile int*)&this->value, -decr);
    }

    volatile int value;
};

struct alignas(std::hardware_destructive_interference_size) AtomicInt64
{
    /// Constructor
    AtomicInt64()
        : value(0)
    {}

    /// Constructor
    AtomicInt64(int64 initial)
        : value(initial)
    {}

    /// Assignment (unsafe)
    void operator=(int64 value)
    {
        this->value = value;
    }

    /// Add 
    int64 Add(int64 add) 
    {
        return Threading::Interlocked::Add((volatile int64*)&this->value, add);
    }
    /// Subtract
    int64 Sub(int64 sub)
    {
        return Threading::Interlocked::Add((volatile int64*)&this->value, -sub);
    }
    /// Or
    int64 Or(int64 mask)
    {
        return Threading::Interlocked::Or((volatile int64*)&this->value, mask);
    }
    /// And
    int64 And(int64 mask)
    {
        return Threading::Interlocked::And((volatile int64*)&this->value, mask);
    }
    /// Exchange
    int64 Exchange(int64 value)
    {
        return Threading::Interlocked::Exchange((volatile int64*)&this->value, value);
    }
    /// Compare and exchange
    int64 CompareExchange(int64 exchange, int64 comparand)
    {
        return Threading::Interlocked::CompareExchange((volatile int64*)&this->value, exchange, comparand);
    }
    /// Increment and return new value
    int64 Increment(int64 incr)
    {
        return Threading::Interlocked::Add((volatile int64*)&this->value, incr);
    }
    /// Decrement and return new value
    int64 Decrement(int64 decr)
    {
        return Threading::Interlocked::Add((volatile int64*)&this->value, -decr);
    }

    volatile int64 value;
};

struct alignas(std::hardware_destructive_interference_size) AtomicPointer
{
    /// Constructor
    AtomicPointer(void* initial)
        : ptr(initial)
    {}

    /// Assignment (unsafe)
    void operator=(void* value)
    {
        this->ptr = value;

    }
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
    volatile void* ptr;
};

/// Atomic value only used to decrement, increment and read a 32 bit value
struct alignas(std::hardware_destructive_interference_size) AtomicCounter
{
    volatile int counter;

    AtomicCounter()
        : counter(0)
    {}

    /// Constructor
    AtomicCounter(int initial)
        : counter(initial)
    {}

    /// Equals
    bool operator==(int value) const
    {
        return this->counter == value;
    }

    /// Not-equals
    bool operator!=(int value) const
    {
        return this->counter != value;
    }

    /// Greater
    bool operator>(int value) const
    {
        return this->counter > value;
    }

    /// Greater or equal
    bool operator>=(int value) const
    {
        return this->counter >= value;
    }

    /// Less
    bool operator<(int value) const
    {
        return this->counter < value;
    }

    /// Less or equal
    bool operator<=(int value) const
    {
        return this->counter <= value;
    }

    /// Increment and return the new value
    int Increment()
    {
        return Threading::Interlocked::Increment(&this->counter);
    }

    /// Decrement and return the new value
    int Decrement()
    {
        return Threading::Interlocked::Decrement(&this->counter);
    }

    /// Add and return the old value
    int Add(int addend)
    {
        return Threading::Interlocked::Add(&this->counter, addend);
    }

    /// Subtract and return the old value
    int Subtract(int subtractor)
    {
        return Threading::Interlocked::Add(&this->counter, -subtractor);
    }

    /// Exchange and return the old value
    int Exchange(int value)
    {
        return Threading::Interlocked::Exchange(&this->counter, value);
    }

    /// Unsafe decrement, only use before or after contention is possible
    void RaceDecrement()
    {
        this->counter--;
    }

    /// Unsafe increment, only use before or after contention is possible
    void RaceIncrement()
    {
        this->counter++;
    }

    /// Unsafe exchange, only use before or after contention is possible
    void RaceExchange(int value)
    {
        this->counter = value;
    }
};

/// Atomic value only used to decrement, increment and read a 64 bit value
struct alignas(std::hardware_destructive_interference_size) AtomicCounter64
{
    volatile int64 counter;

    AtomicCounter64()
        : counter(0)
    {}

    /// Constructor
    AtomicCounter64(int64 initial)
        : counter(initial)
    {}

    /// Equals
    bool operator==(int64 value) const
    {
        return this->counter == value;
    }

    /// Not-equals
    bool operator!=(int64 value) const
    {
        return this->counter != value;
    }

    /// Greater
    bool operator>(int64 value) const
    {
        return this->counter > value;
    }

    /// Greater or equal
    bool operator>=(int64 value) const
    {
        return this->counter >= value;
    }

    /// Less
    bool operator<(int64 value) const
    {
        return this->counter < value;
    }

    /// Less or equal
    bool operator<=(int64 value) const
    {
        return this->counter <= value;
    }

    /// Increment and return the new value
    int Increment()
    {
        return Threading::Interlocked::Increment(&this->counter);
    }

    /// Decrement and return the new value
    int Decrement()
    {
        return Threading::Interlocked::Decrement(&this->counter);
    }

    /// Add and return the old value
    int64 Add(int64 addend)
    {
        return Threading::Interlocked::Add(&this->counter, addend);
    }

    /// Subtract and return the old value
    int64 Subtract(int64 subtractor)
    {
        return Threading::Interlocked::Add(&this->counter, -subtractor);
    }

    /// Exchange and return the old value
    int64 Exchange(int64 value)
    {
        return Threading::Interlocked::Exchange(&this->counter, value);
    }

    /// Unsafe decrement, only use before or after contention is possible
    void RaceDecrement()
    {
        this->counter--;
    }

    /// Unsafe increment, only use before or after contention is possible
    void RaceIncrement()
    {
        this->counter++;
    }

    /// Unsafe exchange, only use before or after contention is possible
    void RaceExchange(int64 value)
    {
        this->counter = value;
    }
};


} // namespace Interlocked
} // namespace Threading
