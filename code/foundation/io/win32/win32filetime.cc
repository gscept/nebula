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
Win32FileTime::GetHighBits()
{
    return this->time.dwHighDateTime;
}

//------------------------------------------------------------------------------
/**
*/
uint 
Win32FileTime::GetLowBits()
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
} // namespace Win32