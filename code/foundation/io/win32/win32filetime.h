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
    uint GetHighBits();
    /// get low bits
    uint GetLowBits();
    /// set bits
    void SetBits(uint lowBits, uint highBits);

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
