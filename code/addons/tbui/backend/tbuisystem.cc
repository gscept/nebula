#include "core/sysfunc.h"
#include "timing/calendartime.h"

#undef PostMessage
#include "tb_system.h"

#ifdef TB_RUNTIME_DEBUG_INFO

void
TBDebugOut(const char* str)
{
    Core::SysFunc::DebugOut(str);
}

#endif // TB_RUNTIME_DEBUG_INFO

namespace tb
{
namespace
{
static double
GetMillisecondsSinceEpoch(
    Timing::CalendarTime calendarTime
)
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
}

/** Get the system time in milliseconds since some undefined epoch. */
double
TBSystem::GetTimeMS()
{
    // todo: This is just grabbing the millisecond component of the current time
    Timing::CalendarTime calendarTime = Timing::CalendarTime::GetSystemTime();

    // hack function to work around above
    // Maybe devs will add a native method in the engine to grab this
    return GetMillisecondsSinceEpoch(calendarTime);
}

/** Called when the need to call TBMessageHandler::ProcessMessages has changed due to changes in the
  message queue. fire_time is the new time is needs to be called.
  It may be 0 which means that ProcessMessages should be called asap (but NOT from this call!)
  It may also be TB_NOT_SOON which means that ProcessMessages doesn't need to be called. */
void
TBSystem::RescheduleTimer(double fire_time)
{
}

/** Get how many milliseconds it should take after a touch down event should generate a long click
  event. */
int
TBSystem::GetLongClickDelayMS()
{
    return 500;
}

/** Get how many pixels of dragging should start panning scrollable widgets. */
int
TBSystem::GetPanThreshold()
{
    return 40;
}

/** Get how many pixels a typical line is: The length that should be scrolled when turning a mouse
  wheel one notch. */
int
TBSystem::GetPixelsPerLine()
{
    return 40;
}

/** Get Dots Per Inch for the main screen. */
int
TBSystem::GetDPI()
{
    return 96;
}
} // namespace tb
