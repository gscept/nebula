#pragma once
//------------------------------------------------------------------------------
/**
    @class Timing::CalendarTime
    
    Allows to obtain the current point in time as year, month, day, etc...
    down to milliseconds, convert between filetime and CalendarTime, and
    format the time to a human readable string.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__)
#include "timing/win360/win360calendartime.h"
namespace Timing
{
class CalendarTime : public Win360::Win360CalendarTime
{ };
}
#elif __WII__
#include "timing/wii/wiicalendartime.h"
namespace Timing
{
class CalendarTime : public Wii::WiiCalendarTime
{ };
}
#elif __PS3__
#include "timing/ps3/ps3calendartime.h"
namespace Timing
{
class CalendarTime : public PS3::PS3CalendarTime
{ };
}
#elif __linux__
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
