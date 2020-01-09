#pragma once
#ifndef WIN360_WIN360CALENDARTIME_H
#define WIN360_WIN360CALENDARTIME_H
//------------------------------------------------------------------------------
/**
    @class Win360::Win360CalendarTime
  
    Win360 implementation of CalendarTime.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "timing/base/calendartimebase.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360CalendarTime : public Base::CalendarTimeBase
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

} // namespace Win360
//------------------------------------------------------------------------------
#endif
