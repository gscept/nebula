#pragma once
//------------------------------------------------------------------------------
/**
    @class Dynui::ImguiConsoleHandler
    
    consolehandler that prints its output to the imgui console
    
    (C) 2015-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "dynui/console/imguiconsole.h"
#include "io/consolehandler.h"
#include "util/string.h"

namespace Dynui
{
class ImguiConsoleHandler : public IO::ConsoleHandler
{
    __DeclareClass(ImguiConsoleHandler);
public:
    /// constructor
    ImguiConsoleHandler();
    /// destructor
    virtual ~ImguiConsoleHandler();

    /// attach to main console
    void Setup();
    /// remove from main console
    void Discard();
    /// called by console when attached
    virtual void Open();
    /// called by console when removed
    virtual void Close();
    /// called by console to output data
    virtual void Print(const Util::String& s);
    /// called by console with serious error
    virtual void Error(const Util::String& s);
    /// called by console to output warning
    virtual void Warning(const Util::String& s);    
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s);

};
} // namespace Dynui