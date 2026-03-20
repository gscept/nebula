#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32FileTime
    
    Implements a Win32-specific file-access time stamp.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32FileTime
{
public:
    /// constructor
    Win32FileTime();
    /// construct from string
    Win32FileTime(const Util::String& str);
    /// construct from epoch time
    Win32FileTime(uint64_t epochTime);
    /// operator ==
    friend bool operator==(const Win32FileTime& a, const Win32FileTime& b);
    /// operator !=
    friend bool operator!=(const Win32FileTime& a, const Win32FileTime& b);
    /// operator >
    friend bool operator>(const Win32FileTime& a, const Win32FileTime& b);
    /// operator <
    friend bool operator<(const Win32FileTime& a, const Win32FileTime& b);
    /// convert to string
    Util::String AsString() const;

    /// get high bits
    uint GetHighBits() const;
    /// get low bits
    uint GetLowBits() const;
    /// set bits
    void SetBits(uint lowBits, uint highBits);

    /// convert to epoch time
    uint64_t AsEpochTime() const;
    /// set from epoch time
    void SetFromEpochTime(uint64_t epochTime);

    FILETIME time;
};

//------------------------------------------------------------------------------
/**
*/
inline
Win32FileTime::Win32FileTime()
{
    time.dwLowDateTime = 0;
    time.dwHighDateTime = 0;
}
//------------------------------------------------------------------------------
/**
*/
inline
Win32FileTime::Win32FileTime(uint64_t epochTime)
{
    SetFromEpochTime(epochTime);
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator==(const Win32FileTime& a, const Win32FileTime& b)
{
    return (0 == CompareFileTime(&(a.time), &(b.time)));
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator!=(const Win32FileTime& a, const Win32FileTime& b)
{
    return (0 != CompareFileTime(&(a.time), &(b.time)));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>(const Win32FileTime& a, const Win32FileTime& b)
{
    return (1 == CompareFileTime(&(a.time), &(b.time)));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator <(const Win32FileTime& a, const Win32FileTime& b)
{
    return (-1 == CompareFileTime(&(a.time), &(b.time)));
}

}; // namespace Win32
//------------------------------------------------------------------------------
