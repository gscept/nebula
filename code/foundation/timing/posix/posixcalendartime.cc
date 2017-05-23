//------------------------------------------------------------------------------
//  posixcalendartime.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "timing/calendartime.h"
#include <sys/time.h>

namespace Posix
{
using namespace Timing;
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
CalendarTime
PosixCalendarTime::FromPosixSystemTime(const timeval & t)
{
    CalendarTime calTime;
    struct tm st;
    gmtime_r(&(t.tv_sec), &st);
    calTime.year    = st.tm_year;
    calTime.month   = (Month)(st.tm_mon + 1);
    calTime.weekday = (Weekday)st.tm_wday;;
    calTime.day     = st.tm_mday;
    calTime.hour    = st.tm_hour;
    calTime.minute  = st.tm_min;
    calTime.second  = st.tm_sec;
    calTime.milliSecond = t.tv_usec / 1000;
    return calTime;
}

//------------------------------------------------------------------------------
/**
*/
timeval
PosixCalendarTime::ToPosixSystemTime(const CalendarTime& calTime)
{
    struct tm t;
    t.tm_year         = calTime.year;
    t.tm_mon          = calTime.month;
    t.tm_wday         = calTime.weekday;
    t.tm_mday         = calTime.day;
    t.tm_hour         = calTime.hour;
    t.tm_min          = calTime.minute;
    t.tm_sec          = calTime.second;
    timeval tv;
    tv.tv_sec = mktime(&t);
    tv.tv_usec = calTime.milliSecond * 1000;
    return tv;
}

//------------------------------------------------------------------------------
/**
    Obtains the current system time. This does not depend on the current
    time zone.
*/
CalendarTime
PosixCalendarTime::GetSystemTime()
{
    timeval t;
    gettimeofday(&t, NULL);
    return FromPosixSystemTime(t);
}

//------------------------------------------------------------------------------
/**
    Obtains the current local time (with time-zone adjustment).
*/
CalendarTime
PosixCalendarTime::GetLocalTime()
{
    timeval t;
    gettimeofday(&t, NULL);
    struct tm localTime;
    localtime_r(&(t.tv_sec), &localTime);
    t.tv_sec = mktime(&localTime);    
    return FromPosixSystemTime(t);
}

//------------------------------------------------------------------------------
/**
*/
FileTime
PosixCalendarTime::SystemTimeToFileTime(const CalendarTime& systemTime)
{
    timeval t = ToPosixSystemTime(systemTime);
    FileTime fileTime;
    TIMEVAL_TO_TIMESPEC(&t, &(fileTime.time));
    return fileTime;
}

//------------------------------------------------------------------------------
/**
*/
CalendarTime
PosixCalendarTime::FileTimeToSystemTime(const FileTime& fileTime)
{
    timeval t;
    TIMESPEC_TO_TIMEVAL(&t, &(fileTime.time));
    return FromPosixSystemTime(t);
}

//------------------------------------------------------------------------------
/**
*/
FileTime
PosixCalendarTime::LocalTimeToFileTime(const CalendarTime& localTime)
{
    // XXX: This needs fixing.
    FileTime fileTime;
    timeval t = ToPosixSystemTime(localTime);
    TIMEVAL_TO_TIMESPEC(&t, &(fileTime.time));
    return fileTime;
}

//------------------------------------------------------------------------------
/**
*/
CalendarTime
PosixCalendarTime::FileTimeToLocalTime(const FileTime& fileTime)
{
    // XXX: This needs fixing.
    timeval t;
    TIMESPEC_TO_TIMEVAL(&t, &(fileTime.time));
    return FromPosixSystemTime(t);
}

} // namespace Posix