#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ConsolePanel

    The editor console. It is a view of the core's log, because the core owns the buffer and the
    panel is a view of it. What the panel owns is the state a view needs: which message types it
    shows, where it is scrolled, and the command history.

    This is the reference panel for how the framework is meant to be used: it registers its commands
    and keybinds in OnSetup, keeps its own state, and draws only the rows that are on screen, sizing
    the scroll region from the measured heights of the rows it is not drawing.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/core/editorpanel.h"
#include "timing/timer.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class ConsolePanel final : public EditorPanel
{
    __DeclareClass(ConsolePanel)
public:
    ConsolePanel();
    virtual ~ConsolePanel();

    /// draw the log and the command input
    void Draw() override;

    /// run a command line
    void Execute(const Util::String& command);

    /// put a command into the input and open the panel, which is what lets a menu item ask the user
    /// for the part of the command it does not know
    void SeedCommand(const Util::String& command) override;

protected:
    bool OnSetup() override;

private:
    /// which message types the log currently shows
    enum LogFilter
    {
        FilterAll,
        FilterMessage,
        FilterWarning,
        FilterError,
    };

    /// the rows that pass the filter and the height each of them takes, in the order they are
    /// drawn. Rebuilt
    /// when the filter, the buffer or the width changed, since a row's height is decided by how its
    /// message wraps
    void BuildRows(const Util::Array<LogEntry>& entries, float rowWidth);

    /// the style color a message type is drawn with
    ImVec4 FilterColor(LogMessageType type) const;
    /// the tag a message type is drawn with
    const char* EntryTag(LogMessageType type) const;
    /// the message type a filter matches, or -1 if the filter matches everything
    int FilterType(const LogFilter& filter) const;
    /// true if an entry passes the current filter
    bool MatchesFilter(const LogEntry& entry) const;

    /// walk through the command history and return the command that the input should show
    const Util::String& RecallCommand(int direction);

    /// the input callback. Immediate mode has no element to attach a handler to, so the callback is
    /// what the
    /// history is read through
    static int InputCallback(ImGuiInputTextCallbackData* data);

    /// commands
    void CommandHelp(const Util::Array<Util::String>& args);
    void CommandClear(const Util::Array<Util::String>& args);
    void CommandHistory(const Util::Array<Util::String>& args);
    void CommandListCVars(const Util::Array<Util::String>& args);

    LogFilter activeFilter;

    /// the command input. Immediate mode needs a buffer that survives between frames
    static const IndexT MaxInputLength = 256;
    char inputBuffer[MaxInputLength];
    /// the frame a seeded command is drawn with. The focus is asked for next to the field, and
    /// immediate mode has no element to ask
    bool inputFocusRequested;

    Util::Array<Util::String> previousCommands;
    IndexT previousCommandIndex;
    Util::String emptyCommand;

    /// the rows that pass the filter
    Util::Array<IndexT> visible;
    /// where each visible row starts, with one past the end of the last row appended, so the last
    /// element is
    /// the total height of the list
    Util::Array<float> rowTops;
    float totalRowHeight;
    /// the width the row heights were measured at. They are wrong for any other width, since that
    /// is where
    /// the messages wrap
    float measuredRowWidth;
    IndexT lastLogSize;
    bool visibleDirty;

    /// follow the log when new entries arrive, but only if the log is not scrolled up
    bool scrollToBottom;
    float lastMaxScroll;
};

} // namespace Editor
