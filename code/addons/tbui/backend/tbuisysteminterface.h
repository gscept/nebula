//------------------------------------------------------------------------------
//  backend/tbuisystem.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "core/sysfunc.h"
#include "timing/calendartime.h"
#include "tbuicontext.h"
#include "platform/tb_system_interface.h"

namespace TBUI
{
class TBUISystemInterface : public tb::TBSystemInterface
{
public:
    void DebugOut(const char* str) override;

    double GetTimeMS() override;
    void RescheduleTimer(double fire_time) override;
    int GetLongClickDelayMS() override;
    int GetPanThreshold() override;
    int GetPixelsPerLine() override;
    int GetDPI() override;
};
}
