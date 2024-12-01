//------------------------------------------------------------------------------
//  backend/tbuisystem.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "core/sysfunc.h"
#include "timing/calendartime.h"
#include "tbuicontext.h"
#include "tbuisysteminterface.h"

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
    return TBUI::TBUIContext::state.timer->GetTicks();
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
