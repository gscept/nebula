#pragma once
//------------------------------------------------------------------------------
/**
    @class Win360::Win360FileTime
    
    Implements a Win32/Xbox360-specific file-access time stamp.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360FileTime
{
public:
    /// constructor
    Win360FileTime();
    /// construct from string
    Win360FileTime(const Util::String& str);
    /// operator ==
    friend bool operator==(const Win360FileTime& a, const Win360FileTime& b);
    /// operator !=
    friend bool operator!=(const Win360FileTime& a, const Win360FileTime& b);
    /// operator >
    friend bool operator>(const Win360FileTime& a, const Win360FileTime& b);
    /// operator <
    friend bool operator<(const Win360FileTime& a, const Win360FileTime& b);
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
Win360FileTime::Win360FileTime()
{
    time.dwLowDateTime = 0;
    time.dwHighDateTime = 0;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator==(const Win360FileTime& a, const Win360FileTime& b)
{
    return (0 == CompareFileTime(&(a.time), &(b.time)));
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
operator!=(const Win360FileTime& a, const Win360FileTime& b)
{
    return (0 != CompareFileTime(&(a.time), &(b.time)));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator>(const Win360FileTime& a, const Win360FileTime& b)
{
    return (1 == CompareFileTime(&(a.time), &(b.time)));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
operator <(const Win360FileTime& a, const Win360FileTime& b)
{
    return (-1 == CompareFileTime(&(a.time), &(b.time)));
}

}; // namespace Win360
//------------------------------------------------------------------------------
