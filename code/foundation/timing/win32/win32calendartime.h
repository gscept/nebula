#pragma once
#ifndef WIN32_WIN32CALENDARTIME_H
#define WIN32_WIN32CALENDARTIME_H
//------------------------------------------------------------------------------
/**
    @class Win32::Win32CalendarTime
  
    Win implementation of CalendarTime.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "timing/base/calendartimebase.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32CalendarTime : public Base::CalendarTimeBase
{
public:
    /// get the current system time
    static Timing::CalendarTime GetSystemTime();
    /// get the current local time
    static Timing::CalendarTime GetLocalTime();
    /// convert system time to file time
    static IO::FileTime SystemTimeToFileTime(const Timing::CalendarTime& systemTime);
    /// convert file time to system time
    static Timing::CalendarTime FileTimeToSystemTime(const IO::FileTime& fileTime);
    /// convert local time to file time
    static IO::FileTime LocalTimeToFileTime(const Timing::CalendarTime& localTime);
    /// convert file time to local time
    static Timing::CalendarTime FileTimeToLocalTime(const IO::FileTime& fileTime);

private:
    /// convert from Win32 SYSTEMTIME
    static Timing::CalendarTime FromWin32SystemTime(const SYSTEMTIME& t);
    /// convert to Win32 SYSTEMTIME
    static SYSTEMTIME ToWin32SystemTime(const Timing::CalendarTime& calTime);
};

} // namespace Win32
//------------------------------------------------------------------------------
#endif
