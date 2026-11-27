//------------------------------------------------------------------------------
//  @file commandpalette.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/widgets/commandpalette.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/input/editorkeybinds.h"
#include "core/cvar.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::CommandPalette, 'CPAL', Core::RefCounted);

// The popup is named by this, so the open and the begin have to say the same thing
static const char* CommandPalettePopup = "command_palette";

// The size the popup takes on appearing. The input and the hint are fixed and the list is the part
// that
// scrolls, so the height is what makes the list worth having
static const float PaletteWidth = 560.0f;
static const float PaletteHeight = 400.0f;

// The width the name costs inside a row, which is what makes the descriptions line up between rows.
// A name
// longer than the column pushes its own description out rather than overlapping it
static const float NameColumnWidth = 170.0f;

// What a line is read through. The space a fill leaves is the room for what is typed next rather
// than part of
// the command, and a tab or a newline is the same thing typed
static const char* Whitespace = " \t\n\r";

// The label each group of rows gets, in the order the groups are drawn
static const char* GroupNames[] = { "Recent", "Commands", "Console variables" };

//------------------------------------------------------------------------------
/**
    The filter is fuzzy: the characters of the filter have to appear in the name in the same order,
    but not next to each other, so 'plr' matches 'Player'. The same rule as the outline's filter,
    and only the name is read, because what a row says about itself is not a second way to be found.

    What the check returns is how close the match is, in the order a fuzzy list is read by: the name
    that is the filter, then the name that starts with it, then the match whose characters are
    closest to each other, then the one that starts earliest, and a shorter name before a longer
    one. 'cop' scores 'copy' and does not reach 'cd' at all, which is what makes the check read as a
    search rather than as a bag of letters.
*/
static IndexT
FuzzyScore(const Util::String& name, const Util::String& filter)
{
    if (filter.IsEmpty())
        return 0;
    if (name.IsEmpty())
        return InvalidIndex;

    IndexT firstMatch = InvalidIndex;
    IndexT lastMatch = InvalidIndex;
    IndexT gaps = 0;

    const char* nameCursor = name.AsCharPtr();
    const char* filterCursor = filter.AsCharPtr();
    IndexT pos = 0;
    while (*nameCursor != '\0' && *filterCursor != '\0')
    {
        if (Util::String::ToLower(*nameCursor) == Util::String::ToLower(*filterCursor))
        {
            if (firstMatch == InvalidIndex)
                firstMatch = pos;
            else
                gaps += pos - lastMatch - 1;
            lastMatch = pos;
            filterCursor++;
        }
        pos++;
        nameCursor++;
    }

    if (*filterCursor != '\0')
        return InvalidIndex;

    return gaps * 4 + firstMatch + ((IndexT)name.Length() - (IndexT)filter.Length());
}

//------------------------------------------------------------------------------
/**
    A cvar value as a string, which is what a row says for a cvar that has nothing to say about
    itself
*/
static Util::String
CVarValueAsString(Core::CVar* cVar)
{
    switch (Core::CVarGetType(cVar))
    {
    case Core::CVar_Int:
        return Util::String::FromInt(Core::CVarReadInt(cVar));
    case Core::CVar_Float:
        return Util::String::FromFloat(Core::CVarReadFloat(cVar));
    case Core::CVar_String:
        return Util::String(Core::CVarReadString(cVar));
    default:
        break;
    }
    return Util::String();
}

//------------------------------------------------------------------------------
/**
*/
CommandPalette::CommandPalette() :
    core(nullptr),
    visible(false),
    openRequested(false),
    closeRequested(false),
    inputFocusRequested(false),
    prefillApplied(false),
    highlightedRow(InvalidIndex),
    scrollToHighlight(false)
{
    this->inputBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
*/
CommandPalette::~CommandPalette()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The palette is built with the core, which is what draws it every frame. It is never visible
    until asked.

    The command and the keybind are registered here, which is what makes Ctrl+Shift+P the same route
    as typing 'palette' in the console.
*/
bool
CommandPalette::Setup(Shell* core)
{
    this->core = core;

    core->GetCommands().Register("palette", "open the command palette",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<CommandPalette, &CommandPalette::HandleShow>(this));
    core->GetKeybinds().Register("palette.open", Input::Key::P, Input::Modifier::ControlModifier | Input::Modifier::ShiftModifier, "palette", false);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
CommandPalette::Discard()
{
    this->rows.Clear();
    this->history.Clear();
    this->pendingPrefill.Clear();
    this->pendingCommand.Clear();
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
CommandPalette::HandleShow(const Util::Array<Util::String>& args)
{
    (void)args;
    this->Show();
}

//------------------------------------------------------------------------------
/**
*/
void
CommandPalette::Show()
{
    this->Reset();
    this->inputFocusRequested = true;

    this->visible = true;
    this->openRequested = true;
    this->closeRequested = false;
}

//------------------------------------------------------------------------------
/**
    The rows the filter leaves, in the order they are drawn. The list is built again every frame,
    because the filter is what decides which of the entries are in it, and the highlight is an index
    into it.

    While nothing is typed the list is the three groups: what the palette ran, in the order it ran
    it, then the registry, then the cvars, each in name order. While something is typed the list is
    one ranking, because the closest match is what a user is looking for and a group that comes
    before it is not a help.
*/
void
CommandPalette::BuildRows(const Util::String& filter)
{
    this->rows.Clear();

    const EditorCommandSystem& registry = this->core->GetCommands();
    Util::Array<Entry> commands;
    for (IndexT i = 0; i < registry.GetCommands().Size(); i++)
    {
        const EditorCommand& command = registry.GetCommands()[i];

        Entry entry;
        entry.name = command.name;
        entry.description = command.description;
        entry.command = command.name + " ";
        entry.group = Command;
        entry.score = FuzzyScore(entry.name, filter);
        commands.Append(entry);
    }

    Util::Array<Entry> cvars;
    for (Core::CVar* cVar = Core::CVarsBegin(); cVar != Core::CVarsEnd(); cVar = Core::CVarNext(cVar))
    {
        const char* description = Core::CVarGetDescription(cVar);

        Entry entry;
        entry.name = Core::CVarGetName(cVar);
        if (description != nullptr && description[0] != '\0')
            entry.description = Util::String(description);
        else
            entry.description = Util::String("=") + CVarValueAsString(cVar);
        entry.command = entry.name + " ";
        entry.group = CVar;
        entry.score = FuzzyScore(entry.name, filter);
        cvars.Append(entry);
    }

    if (filter.IsEmpty())
    {
        for (IndexT i = 0; i < this->history.Size(); i++)
        {
            Entry entry;
            entry.name = this->history[i];
            entry.description = Util::String();
            entry.command = this->history[i] + " ";
            entry.group = Recent;
            entry.score = 0;
            this->rows.Append(entry);
        }

        commands.SortWithFunc(&CommandPalette::EntryNameOrder);
        for (IndexT i = 0; i < commands.Size(); i++)
            this->rows.Append(commands[i]);

        cvars.SortWithFunc(&CommandPalette::EntryNameOrder);
        for (IndexT i = 0; i < cvars.Size(); i++)
            this->rows.Append(cvars[i]);
    }
    else
    {
        for (IndexT i = 0; i < commands.Size(); i++)
        {
            if (commands[i].score != InvalidIndex)
                this->rows.Append(commands[i]);
        }

        for (IndexT i = 0; i < cvars.Size(); i++)
        {
            if (cvars[i].score != InvalidIndex)
                this->rows.Append(cvars[i]);
        }

        this->rows.SortWithFunc(&CommandPalette::EntryRankOrder);
    }
}

//------------------------------------------------------------------------------
/**
    Closest first, and a tie broken by name so that two entries that are equally close do not move
    around between frames
*/
bool
CommandPalette::EntryRankOrder(const Entry& lhs, const Entry& rhs)
{
    if (lhs.score != rhs.score)
        return lhs.score < rhs.score;

    return Util::String::StrICmp(lhs.name.AsCharPtr(), rhs.name.AsCharPtr()) < 0;
}

//------------------------------------------------------------------------------
/**
    The name order inside a group. The recent rows are in the order they were run, so they are not
    sorted, and this is only ever called on the registry and the cvars
*/
bool
CommandPalette::EntryNameOrder(const Entry& lhs, const Entry& rhs)
{
    return Util::String::StrICmp(lhs.name.AsCharPtr(), rhs.name.AsCharPtr()) < 0;
}

//------------------------------------------------------------------------------
/**
    The wrap is what makes the list a cycle. A row is never something the keyboard has to step over,
    because every entry in the list is something the input can become.
*/
void
CommandPalette::MoveHighlight(int direction)
{
    const SizeT num = this->rows.Size();
    if (num == 0)
    {
        this->highlightedRow = InvalidIndex;
        return;
    }

    IndexT index = this->highlightedRow == InvalidIndex ? 0 : this->highlightedRow + direction;
    index = (IndexT)(((index % num) + num) % num);

    this->highlightedRow = index;
    this->scrollToHighlight = true;
}

//------------------------------------------------------------------------------
/**
    The entry is put in front of the user rather than run, because what a command still lacks is
    usually something only they know: an argument, or the value a cvar is being set to.

    The prefill is queued rather than written, because from the moment a field is focused it keeps
    its own copy of the text and reads the buffer only when it is activated. The callback applies it
    while the field owns the text, and the buffer is what a field that is not focused reads.
*/
void
CommandPalette::Prefill(const Entry& entry)
{
    this->pendingPrefill = entry.command;
    this->inputFocusRequested = true;
}

//------------------------------------------------------------------------------
/**
    The callback runs inside the input widget, so the text is written through the callback data
    rather than through the palette.
*/
int
CommandPalette::InputCallback(ImGuiInputTextCallbackData* data)
{
    CommandPalette* palette = static_cast<CommandPalette*>(data->UserData);

    if (!palette->pendingPrefill.IsEmpty())
    {
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, palette->pendingPrefill.AsCharPtr());
        palette->pendingPrefill.Clear();
        palette->prefillApplied = true;
    }

    // A fill is not a selection. The caret goes to the end of what the field holds, because what is
    // typed next
    // is the argument, not a replacement of the word that was just filled. ImGui selects the whole
    // field when it
    // activates it through the keyboard, and an Enter on an empty input ends the field, so a fill
    // lands on the
    // frame the field is activated again and everything is marked
    if (palette->prefillApplied || data->EventActivated)
        data->SetSelection(data->BufTextLen, data->BufTextLen);

    return 0;
}

//------------------------------------------------------------------------------
/**
    What a palette runs changes the model, so the command is queued and runs after the frame that
    drew it, the same way a panel queues what it wants changed.
*/
void
CommandPalette::Submit(const Util::String& command)
{
    if (command.IsEmpty() || this->core == nullptr)
        return;

    // What the line is trimmed of before it is run and recorded, so a fill that was not finished
    // does not leave
    // its trailing space in the command
    Util::String line = command;
    line.Trim(Whitespace);
    if (line.IsEmpty())
        return;

    // A line the palette would have to run and then fail on is not worth closing for, so it stays
    // where it is
    if (!this->IsValidLine(line))
        return;

    this->RecordCommand(line);

    this->pendingCommand = line;
    this->closeRequested = true;
    this->core->QueueAction(Util::Delegate<void()>::FromMethod<CommandPalette, &CommandPalette::RunCommand>(this));
}

//------------------------------------------------------------------------------
/**
*/
void
CommandPalette::RunCommand()
{
    if (this->core == nullptr)
        return;

    const Util::String command = this->pendingCommand;
    this->pendingCommand.Clear();

    this->core->Log(LogMessageType::N_INPUT, command);
    this->core->ExecuteCommand(command);
}

//------------------------------------------------------------------------------
/**
    The first word is what the command system looks up, so that is the word the palette checks, and
    it has to be the whole word: a line that only begins with 'set' is not the set command. The '>'
    the cvar evaluation takes off is part of how a cvar line is written, so a line that starts with
    it is checked as a cvar name
*/
bool
CommandPalette::IsValidLine(const Util::String& line) const
{
    if (this->core == nullptr)
        return false;

    Util::Array<Util::String> splits = line.Tokenize(" ");
    if (splits.IsEmpty())
        return false;

    const char* verb = splits[0].AsCharPtr();
    if (verb[0] == '>')
        verb = verb + 1;

    const EditorCommandSystem& registry = this->core->GetCommands();
    for (IndexT i = 0; i < registry.GetCommands().Size(); i++)
    {
        if (Util::String::StrICmp(verb, registry.GetCommands()[i].name.AsCharPtr()) == 0)
            return true;
    }

    return Core::CVarGet(verb) != nullptr;
}

//------------------------------------------------------------------------------
/**
    The line is remembered for the next open. The same line twice is one row, so a repeat moves it
    to the front rather than doubling it, which is what a history that is used for repeating looks
    like
*/
void
CommandPalette::RecordCommand(const Util::String& command)
{
    for (IndexT i = 0; i < this->history.Size(); i++)
    {
        if (this->history[i] == command)
        {
            for (IndexT j = i; j > 0; j--)
                this->history[j] = this->history[j - 1];
            this->history[0] = command;
            return;
        }
    }

    if ((IndexT)this->history.Size() >= MaxHistory)
        this->history.EraseBack();

    this->history.Insert(0, command);
}

//------------------------------------------------------------------------------
/**
*/
void
CommandPalette::Reset()
{
    this->inputBuffer[0] = 0;
    this->pendingPrefill.Clear();
    this->prefillApplied = false;
    this->rows.Clear();
    this->highlightedRow = InvalidIndex;
    this->scrollToHighlight = false;
}

//------------------------------------------------------------------------------
/**
    The popup centers on the viewport and takes its size on appearing, so a palette the user resized
    keeps that shape until the layout is reset. The input stays at the top and the hint at the
    bottom, and the list is the scroll region in between.
*/
void
CommandPalette::Draw()
{
    if (!this->visible)
        return;

    if (this->openRequested)
    {
        ImGui::OpenPopup(CommandPalettePopup);
        this->openRequested = false;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(PaletteWidth, PaletteHeight), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal(CommandPalettePopup, &open))
    {
        // Ctrl+Shift+P is the registered keybind, and a keybind only fires when ImGui did not take
        // the key,
        // which is what keeps it from running while a text field has focus. A field having focus is
        // exactly the
        // situation a palette is usually wanted in, so the palette takes the chord itself when it
        // does. When no
        // field has focus the keybind has already run, and the chord is not taken twice. This is
        // done before the
        // input, so that the open clears what the other field holds before anything is drawn
        if (ImGui::GetIO().WantTextInput && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, ImGuiInputFlags_RouteGlobal))
            this->Show();

        if (this->inputFocusRequested)
        {
            ImGui::SetKeyboardFocusHere();
            this->inputFocusRequested = false;
        }

        this->prefillApplied = false;

        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        const bool edited = ImGui::InputTextWithHint("##command", "Type a command or a cvar...",
            this->inputBuffer, MaxInputLength, ImGuiInputTextFlags_CallbackAlways, &CommandPalette::InputCallback, this);
        ImGui::PopItemWidth();

        // The field was not focused, so the prefill the callback could not apply is applied to the
        // buffer
        // here, and the field reads the buffer when it is focused again. The caret is put at the
        // end of it by the
        // callback on that frame, because ImGui selects the whole field when it activates it
        // through the keyboard
        if (!this->pendingPrefill.IsEmpty())
        {
            this->pendingPrefill.CopyToBuffer(this->inputBuffer, MaxInputLength);
            this->pendingPrefill.Clear();
            this->prefillApplied = true;
        }

        // The trailing space a prefill leaves is the room for what is typed next, not part of the
        // filter
        Util::String filter = Util::String(this->inputBuffer);
        filter.Trim(Whitespace);

        this->BuildRows(filter);

        // The groups are only what the list is while nothing is typed. While something is typed the
        // rows are
        // ranked, and a cvar that is the closest match does not wait for its group
        const bool ranked = !filter.IsEmpty();

        // A letter moves the selection to the top of what the filter leaves, because the list is
        // read through
        // the filter. A fill is not a letter, so the row that was filled stays where it is, and a
        // selection that
        // is not in the list is not a selection
        if ((edited && !this->prefillApplied) || this->highlightedRow == InvalidIndex ||
            this->highlightedRow >= (IndexT)this->rows.Size())
        {
            if (!this->rows.IsEmpty())
            {
                this->highlightedRow = 0;
                this->scrollToHighlight = true;
            }
            else
                this->highlightedRow = InvalidIndex;
        }

        // Escape closes. ImGui only closes a popup on Escape when keyboard navigation is enabled,
        // which this
        // editor does not enable, so the palette closes it itself. The route is global, so that the
        // key reaches
        // the popup whether or not its window is the focused one.
        //
        // The rest are taken with IsKeyPressed rather than with Shortcut, because a shortcut that
        // could also be a character is culled while a text item is active, and Tab is one of the
        // keys ImGui counts as a possible character, which is what kept the fill from happening.
        // The input owns the caret keys - left, right, delete, backspace, home and end - and none
        // of these, so they are the palette's while the field has focus
        if (ImGui::Shortcut(ImGuiKey_Escape, ImGuiInputFlags_RouteGlobal))
        {
            this->closeRequested = true;
        }
        else if (ImGui::IsKeyChordPressed(ImGuiMod_Shift | ImGuiKey_Tab))
        {
            // Shift+Tab is the backwards cycle, since plain Tab is the fill now
            this->MoveHighlight(-1);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Tab))
        {
            if (this->highlightedRow != InvalidIndex)
                this->Prefill(this->rows[this->highlightedRow]);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter))
        {
            // An empty input has nothing to run, so Enter takes the row the keyboard is on instead.
            // Once the
            // input has its first word it is a command line, and the next Enter runs what it holds
            if (filter.IsEmpty())
            {
                if (this->highlightedRow != InvalidIndex)
                    this->Prefill(this->rows[this->highlightedRow]);
            }
            else
                this->Submit(filter);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
        {
            this->MoveHighlight(1);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
        {
            this->MoveHighlight(-1);
        }

        ImGui::Separator();

        // The list is a scroll region of its own, so that the input and the hint do not move with
        // it
        const float hint = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("rows", ImVec2(0, -hint));

        const ImGuiStyle& style = ImGui::GetStyle();
        const float lineHeight = ImGui::GetFontSize();
        const float rowWidth = ImGui::GetContentRegionAvail().x;
        IndexT group = InvalidIndex;

        for (IndexT i = 0; i < (IndexT)this->rows.Size(); i++)
        {
            const Entry& entry = this->rows[i];

            if (!ranked && group != (IndexT)entry.group)
            {
                group = (IndexT)entry.group;
                ImGui::SeparatorText(GroupNames[group]);
            }

            const float rowLeft = ImGui::GetCursorPosX();
            const float nameWidth = ImGui::CalcTextSize(entry.name.AsCharPtr()).x;

            // The description starts at the column, or after the name when the name is longer than
            // it, because
            // a row that overlaps itself is not a row
            const float descX = rowLeft + (nameWidth > NameColumnWidth ? nameWidth + style.ItemInnerSpacing.x : NameColumnWidth);
            const float wrapWidth = rowWidth - (descX - rowLeft);
            const float descHeight = wrapWidth > 0.0f ?
                ImGui::CalcTextSize(entry.description.AsCharPtr(), nullptr, false, wrapWidth).y : ImGui::GetTextLineHeight();
            const float rowHeight = descHeight > lineHeight ? descHeight : lineHeight;

            // The highlight is drawn before the row, because the row draws its text over whatever
            // was there
            // before it
            if (i == this->highlightedRow)
            {
                ImVec2 min = ImGui::GetCursorScreenPos();
                ImVec2 max = ImVec2(min.x + rowWidth, min.y + rowHeight);
                ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_Header));

                // The scroll only moves when the highlight moved, so that a scroll done by hand is
                // not fought
                if (this->scrollToHighlight)
                {
                    ImGui::SetScrollHereY();
                    this->scrollToHighlight = false;
                }
            }

            ImGui::BeginGroup();
            ImGui::TextUnformatted(entry.name.AsCharPtr());

            if (!entry.description.IsEmpty())
            {
                ImGui::SameLine(descX);
                ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteMuted));
                ImGui::PushTextWrapPos(rowLeft + rowWidth);
                ImGui::TextUnformatted(entry.description.AsCharPtr());
                ImGui::PopTextWrapPos();
                ImGui::PopStyleColor();
            }

            // The group is the row, so the whole width is clickable rather than only the text on
            // it. A click
            // fills the input the way Tab does
            ImGui::EndGroup();

            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                if (ImGui::IsItemClicked())
                    this->Prefill(entry);
            }
        }

        if (this->rows.IsEmpty())
            ImGui::TextDisabled("Nothing matches the filter");

        ImGui::EndChild();

        // An Enter that does nothing looks like a broken Enter, so the line that says what the keys
        // do also says
        // that the first word is not something the palette has
        const bool validLine = filter.IsEmpty() || this->IsValidLine(filter);
        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(validLine ? PaletteMuted : PaletteError));
        if (validLine)
            ImGui::TextUnformatted("Arrows choose, Tab fills the input, Enter runs it, Escape closes");
        else
            ImGui::TextUnformatted("The first word is not a command or a cvar");
        ImGui::PopStyleColor();

        if (this->closeRequested)
        {
            ImGui::CloseCurrentPopup();
            this->closeRequested = false;
            this->visible = false;
            this->Reset();
        }

        ImGui::EndPopup();
    }

    // ImGui itself closing the popup, Escape through the nav, lands here too
    if (!open)
    {
        this->visible = false;
        this->Reset();
    }
}

} // namespace Editor
