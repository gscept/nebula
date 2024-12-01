//------------------------------------------------------------------------------
//  backend/tbuisystem.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "core/sysfunc.h"
#include "timing/calendartime.h"
#include "tbuicontext.h"
#include "tbuisysteminterface.h"

// hopefully we dont need this, leaving it for now
#if 0
//------------------------------------------------------------------------------
/**
*/
static double
GetMillisecondsSinceEpoch(Timing::CalendarTime calendarTime)
{
    int year = calendarTime.GetYear();
    int month = calendarTime.GetMonth();
    int day = calendarTime.GetDay();
    int hour = calendarTime.GetHour();
    int minute = calendarTime.GetMinute();
    int second = calendarTime.GetSecond();
    int millisecond = calendarTime.GetMilliSecond();

    // Validate input (basic checks)
    if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 ||
        second > 59 || millisecond < 0 || millisecond > 999)
    {
        return -1.0; // Failure
    }

    // Construct a tm structure
    std::tm timeinfo = {};
    timeinfo.tm_year = year - 1900; // tm_year is years since 1900
    timeinfo.tm_mon = month - 1;    // tm_mon is 0-based
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;

    // Convert to time_t (seconds since epoch)
    std::time_t time_since_epoch = std::mktime(&timeinfo);

    // Check for invalid time conversion
    if (time_since_epoch == -1)
    {
        return -1.0; // Failure
    }

    // Convert to milliseconds as double
    return static_cast<double>(time_since_epoch) * 1000.0 + static_cast<double>(millisecond);
}
#endif

namespace TBUI
{
void
TBUISystemInterface::DebugOut(const char* str)
{
    Core::SysFunc::DebugOut(str);
}

double
TBUISystemInterface::GetTimeMS()
{
    n_assert(TBUI::TBUIContext::state.timer.isvalid());
    return TBUI::TBUIContext::state.timer->GetTime();
}

void
TBUISystemInterface::RescheduleTimer(double fire_time)
{
}

int
TBUISystemInterface::GetLongClickDelayMS()
{
    return 500;
}

int
TBUISystemInterface::GetPanThreshold()
{
    return 40;
}

int
TBUISystemInterface::GetPixelsPerLine()
{
    return 40;
}

int
TBUISystemInterface::GetDPI()
{
    return 96;
}
}
