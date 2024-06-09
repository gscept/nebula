#pragma once
//------------------------------------------------------------------------------
/**
    @class Timing::CalendarTime
    
    Allows to obtain the current point in time as year, month, day, etc...
    down to milliseconds, convert between filetime and CalendarTime, and
    format the time to a human readable string.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __WIN32__
#include "timing/win32/win32calendartime.h"
namespace Timing
{
class CalendarTime : public Win32::Win32CalendarTime
{ };
}
#elif __linux__ || __APPLE__
#include "timing/posix/posixcalendartime.h"
namespace Timing
{
class CalendarTime : public Posix::PosixCalendarTime
{ };
}
#else
#error "Timing::CalendarTime not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
