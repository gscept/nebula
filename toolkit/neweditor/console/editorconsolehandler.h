#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditorConsoleHandler

    Console handler that routes the engine console output into the editor log. The editor core
    broadcasts whatever it receives to every panel, so this does not know which panel shows it.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "io/consolehandler.h"
#include "util/string.h"

namespace Editor
{

class Shell;

class EditorConsoleHandler : public IO::ConsoleHandler
{
    __DeclareClass(EditorConsoleHandler);
public:
    EditorConsoleHandler();
    virtual ~EditorConsoleHandler();

    /// attach to the main console and route its output to the editor
    void Setup(Shell* core);
    /// remove from the main console
    void Discard();

    /// called by console when attached
    virtual void Open() override;
    /// called by console when removed
    virtual void Close() override;
    /// called by console to output data
    virtual void Print(const Util::String& s) override;
    /// called by console with serious error
    virtual void Error(const Util::String& s) override;
    /// called by console to output warning
    virtual void Warning(const Util::String& s) override;
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s) override;

private:
    Shell* core;
};

} // namespace Editor
