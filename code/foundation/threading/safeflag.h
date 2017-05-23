#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::SafeFlag
    
    A thread-safe flag variable.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "threading/interlocked.h"

//------------------------------------------------------------------------------
namespace Threading
{
class SafeFlag
{
public:
    /// constructor
    SafeFlag();
    /// set the flag
    void Set();
    /// clear the flag
    void Clear();
    /// test if the flag is set
    bool Test() const;
    /// test if flag is set, if yes, clear flag
    bool TestAndClearIfSet();

private:
    int volatile flag;
};

//------------------------------------------------------------------------------
/**
*/
inline
SafeFlag::SafeFlag() :
    flag(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
SafeFlag::Set()
{
    Interlocked::Exchange(&this->flag, 1);
}

//------------------------------------------------------------------------------
/**
*/
inline void
SafeFlag::Clear()
{
    Interlocked::Exchange(&this->flag, 0);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
SafeFlag::Test() const
{
    return (0 != this->flag);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
SafeFlag::TestAndClearIfSet()
{
    return (1 == Interlocked::CompareExchange(&this->flag, 0, 1));
}

} // namespace Threading
//------------------------------------------------------------------------------
