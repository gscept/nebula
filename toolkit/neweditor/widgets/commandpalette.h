#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::CommandPalette

    This class is the command palette. Ctrl+Shift+P opens it. The input is matched against the
    command registry and the console variables, and Enter runs what the input holds. The core keeps
    one palette and shows it every frame, after the panels.

    A row can be put into the input without running it, so an argument or a value can still be
    typed. While the input is empty, the commands the palette last ran are at the top of the list.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "imgui.h"
#include "util/array.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Editor
{

class Shell;

class CommandPalette final : public Core::RefCounted
{
    __DeclareClass(CommandPalette)
public:
    CommandPalette();
    virtual ~CommandPalette();

    /// Prepare the palette. The core calls this function at Setup, and registers its command and
    /// keybind there.
    bool Setup(Shell* core);
    /// Release what the palette holds.
    void Discard();

    /// Open the palette.
    void Show();

    /// Show the popup. The core calls this function once per frame, inside the ImGui frame.
    void Draw();

private:
    /// which part of the list a row is in. The recent group is only in the list while the input is
    /// empty.
    enum EntryGroup
    {
        Recent = 0,
        Command = 1,
        CVar = 2,
    };

    /// one row of the list. command is what the input gets prefilled with: the name and the space
    /// an argument or a value is typed into.
    struct Entry
    {
        Util::String name;
        /// what the row says after the name. A cvar without a description says what it holds.
        Util::String description;
        Util::String command;
        EntryGroup group;
        /// how close the name is to the filter, or InvalidIndex when the name does not pass it
        IndexT score;
    };

    /// The command that opens the palette, which the keybind runs.
    void HandleShow(const Util::Array<Util::String>& args);

    /// Build the rows the filter leaves, in draw order. It runs every frame, because the filter
    /// decides which rows are in the list and the highlight is an index into it.
    void BuildRows(const Util::String& filter);

    /// Return true when lhs ranks before rhs. The closest name is first, and a tie is broken by
    /// name.
    static bool EntryRankOrder(const Entry& lhs, const Entry& rhs);

    /// Return true when lhs sorts before rhs by name, inside a group.
    static bool EntryNameOrder(const Entry& lhs, const Entry& rhs);

    /// Move the highlight one row. It wraps at the ends.
    void MoveHighlight(int direction);

    /// Put an entry into the input without running it.
    void Prefill(const Entry& entry);

    /// The input callback. Immediate mode has no field to attach a handler to, so a prefill is
    /// applied here. The caret goes to the end, because what is typed next is the argument.
    static int InputCallback(ImGuiInputTextCallbackData* data);

    /// Queue what the input holds and close. A command changes the model, so it runs after the
    /// frame.
    void Submit(const Util::String& command);
    /// Run the queued command.
    void RunCommand();

    /// Return true when the first word of the line is a command or a cvar. The word has to be the
    /// whole word, so a line that only begins with 'set' is not the set command.
    bool IsValidLine(const Util::String& line) const;

    /// Remember a submitted command for the next open. The same line twice is one row, so a repeat
    /// moves it to the front.
    void RecordCommand(const Util::String& command);

    /// Clear the input, the list and the highlight. The history is not cleared, because the next
    /// open is often for a repeat.
    void Reset();

    Shell* core;

    /// true when the palette is open
    bool visible;
    /// true when the palette must open the popup on the next draw
    bool openRequested;
    /// true when the palette must close the popup on the next draw
    bool closeRequested;

    /// The input needs a buffer that stays between frames.
    static const IndexT MaxInputLength = 256;
    char inputBuffer[MaxInputLength];
    /// true when the input gets focus on the next draw
    bool inputFocusRequested;

    /// the prefill waiting for the input callback, because the field keeps its own text while it is
    /// focused
    Util::String pendingPrefill;

    /// true when a prefill was applied this frame. The field reports a fill the same way as a typed
    /// letter, and the two are handled differently: a letter moves the highlight, a fill does not.
    bool prefillApplied;

    /// the command to run, for the queued run
    Util::String pendingCommand;

    /// the commands the palette ran, most recent first. They survive a close, because the next open
    /// is often for a repeat.
    static const IndexT MaxHistory = 5;
    Util::Array<Util::String> history;

    /// the rows the filter leaves
    Util::Array<Entry> rows;
    /// the row the keyboard is on. Mouse hover is ImGui's own highlight, so this is only the
    /// keyboard's.
    IndexT highlightedRow;
    /// true when the highlight moved this frame, so the scroll region moves to it
    bool scrollToHighlight;
};

} // namespace Editor
