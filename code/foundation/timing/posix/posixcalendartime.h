#ifndef POSIX_POSIXCALENDARTIME_H
#define POSIX_POSIXCALENDARTIME_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixCalendarTime
  
    Posix implementation of CalendarTime.
    
    (C) 2007 Radon Labs GmbH
*/    
#include "timing/base/calendartimebase.h"



//------------------------------------------------------------------------------
namespace Posix
{
class PosixCalendarTime : public Base::CalendarTimeBase
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
    /// convert from Posix time_t
    static Timing::CalendarTime FromPosixSystemTime(const timeval & t);
    /// convert to Posix time_t
    static timeval ToPosixSystemTime(const Timing::CalendarTime& calTime);
};

} // namespace Posix
//------------------------------------------------------------------------------
#endif
