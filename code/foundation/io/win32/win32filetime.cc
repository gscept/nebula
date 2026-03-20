//------------------------------------------------------------------------------
//  win32filetime.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/win32/win32filetime.h"

namespace Win32
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
Win32FileTime::Win32FileTime(const String& str)
{
    Array<String> tokens = str.Tokenize("#");
    n_assert(tokens.Size() == 2);
    this->time.dwHighDateTime = tokens[0].AsInt();
    this->time.dwLowDateTime = tokens[1].AsInt();
}

//------------------------------------------------------------------------------
/**
*/
String
Win32FileTime::AsString() const
{
    String str;
    str.Format("%d#%d", this->time.dwHighDateTime, this->time.dwLowDateTime);
    return str;
}

//------------------------------------------------------------------------------
/**
*/
uint 
Win32FileTime::GetHighBits() const
{
    return this->time.dwHighDateTime;
}

//------------------------------------------------------------------------------
/**
*/
uint 
Win32FileTime::GetLowBits() const
{
    return this->time.dwLowDateTime;
}

//------------------------------------------------------------------------------
/**
*/
void 
Win32FileTime::SetBits( uint lowBits, uint highBits )
{
    this->time.dwLowDateTime = lowBits;
    this->time.dwHighDateTime = highBits;
}

//------------------------------------------------------------------------------
/**
*/
uint64_t Win32FileTime::AsEpochTime() const
{
    LONGLONG ll = ((LONGLONG)this->time.dwHighDateTime << 32) | this->time.dwLowDateTime;
    ll -= 116444736000000000; // subtract epoch difference
    ll /= 10000000; // convert from 100-nanosecond intervals to seconds
    return (uint64_t)ll;
}

/// set from epoch time
void Win32FileTime::SetFromEpochTime(uint64_t epochTime)
{
    LONGLONG ll = (LONGLONG)epochTime * 10000000 + 116444736000000000; // convert to 100-nanosecond intervals and add epoch difference
    this->time.dwLowDateTime = (DWORD)(ll & 0xFFFFFFFF);
    this->time.dwHighDateTime = (DWORD)(ll >> 32);
}

} // namespace Win32