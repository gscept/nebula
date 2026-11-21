//------------------------------------------------------------------------------
//  @file consolepanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/panels/consolepanel.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/input/editorkeybinds.h"
#include "neweditor/base/palette.h"
#include "core/cvar.h"

namespace Editor
{

__ImplementClass(Editor::ConsolePanel, 'CNSP', Editor::EditorPanel);

//------------------------------------------------------------------------------
/**
*/
ConsolePanel::ConsolePanel() :
    activeFilter(LogFilter::FilterAll),
    previousCommandIndex(-1),
    inputFocusRequested(false),
    lastLogSize(0),
    visibleDirty(true),
    scrollToBottom(true),
    lastMaxScroll(0.0f),
    totalRowHeight(0.0f),
    measuredRowWidth(0.0f)
{
    this->name = "console";
    this->inputBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
*/
ConsolePanel::~ConsolePanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Commands and keybinds are registered here, which is what makes the console extensible.
*/
bool
ConsolePanel::OnSetup()
{
    EditorCommandSystem& commands = this->core->GetCommands();
    commands.Register("help", "prints how to use the console",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ConsolePanel, &ConsolePanel::CommandHelp>(this));
    commands.Register("clear", "clear the log",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ConsolePanel, &ConsolePanel::CommandClear>(this));
    commands.Register("history", "list the commands that were run this session",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ConsolePanel, &ConsolePanel::CommandHistory>(this));
    commands.Register("list", "list available cvars",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ConsolePanel, &ConsolePanel::CommandListCVars>(this));

    this->core->GetKeybinds().Register("console.help", Input::Key::F1, Input::Modifier::NoModifier, "help");
    this->core->GetKeybinds().Register("console.clear", Input::Key::L, Input::Modifier::ControlModifier, "clear");

    return true;
}

//------------------------------------------------------------------------------
/**
    The rows that pass the filter and where each of them sits in the scroll region. The ones that
    are filtered out cannot be rows at all, which is why the visible set is built separately. A row
    is as tall as its wrapped message makes it, so the heights are measured here, once per change,
    rather than frame by frame. The widths are measured with the same sizes the drawing submits,
    which is what keeps the scroll range in step with the content.
*/
void
ConsolePanel::BuildRows(const Util::Array<LogEntry>& entries, float rowWidth)
{
    if (!this->visibleDirty && entries.Size() == this->lastLogSize && rowWidth == this->measuredRowWidth)
        return;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float lineHeight = ImGui::GetFontSize();

    // rowTops holds where every row starts and one past the end of the last one, so it doubles as
    // the prefix
    // sum and the total height
    this->visible.Clear();
    this->rowTops.Clear();
    this->rowTops.Append(0.0f);

    float top = 0.0f;
    for (IndexT i = 0; i < entries.Size(); i++)
    {
        if (!this->MatchesFilter(entries[i]))
            continue;

        const LogEntry& entry = entries[i];

        // the message starts where the timestamp and the tag leave off, and wraps at the edge of
        // the region
        float x = ImGui::CalcTextSize(entry.timestamp.AsCharPtr()).x + style.ItemSpacing.x;
        x += ImGui::CalcTextSize(this->EntryTag(entry.type)).x + style.ItemSpacing.x;
        float msgHeight = ImGui::CalcTextSize(entry.msg.AsCharPtr(), nullptr, false, rowWidth - x).y;

        this->visible.Append(i);
        top += (msgHeight > lineHeight ? msgHeight : lineHeight) + style.ItemSpacing.y;
        this->rowTops.Append(top);
    }

    this->totalRowHeight = top;
    this->lastLogSize = entries.Size();
    this->measuredRowWidth = rowWidth;
    this->visibleDirty = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ConsolePanel::Draw()
{
    EditorLog* log = this->core->GetLog();
    if (log == nullptr)
        return;

    float avail = ImGui::GetContentRegionAvail().x;
    float xStart = ImGui::GetCursorPosX();

    ImGui::Text("CONSOLE");
    ImGui::SameLine();
    
    // The filter tabs
    const LogFilter tabs[4] = {
        LogFilter::FilterAll,
        LogFilter::FilterMessage,
        LogFilter::FilterWarning,
        LogFilter::FilterError,
    };
    const char* names[4] = { "All", "Message", "Warning", "Error" };


    ImGuiStyle& style = ImGui::GetStyle();
    float size = 0;
    for (int i = 0; i < 4; i++)
    {
        size += ImGui::CalcTextSize(names[i]).x;
    }

    size += style.FramePadding.x * 2.0f;

    float off = (avail - size) * 0.5f;
    if (off > 0.0f)
        ImGui::SetCursorPosX(xStart + off);

    for (int i = 0; i < 4; i++)
    {
        if (this->activeFilter == tabs[i])
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));
            ImGui::Text(names[i]);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextDisabled(names[i]);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
            if (ImGui::IsItemClicked())
            {
                if (tabs[i] != this->activeFilter)
                {
                    this->activeFilter = tabs[i];
                    this->visibleDirty = true;
                }
                
            }
        }
        
        if (i < 3)
            ImGui::SameLine();
    }

    ImGui::SameLine();

    log->Enter();

    const Util::Array<LogEntry>& entries = log->GetEntries();

    {   // Entries counter
        Util::String header;
        header.Format("%d entries", (int)entries.Size());
        float size = ImGui::CalcTextSize(header.AsCharPtr()).x;
        size += style.FramePadding.x * 2.0f;
        float off = (avail - size);
        if (off > 0.0f)
            ImGui::SetCursorPosX(xStart + off);
        
        ImGui::TextUnformatted(header.AsCharPtr());
    }

    ImGui::Separator();

    // The log is a scroll region of its own, so that the header and the input do not move with it.
    // The footer is
    // the input and the separator above it, and the child gets the rest of the window. The
    // separator counts in
    // the layout of its own since 1.92.7, so its size is part of the footer rather than a guess
    const float footer = style.ItemSpacing.y + style.SeparatorSize + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("log", ImVec2(0, -footer));

    // The scroll sizes reflect the layout from last frame, which already includes the rows drawn
    // then
    const float maxScroll = ImGui::GetScrollMaxY();
    const float scrollTop = ImGui::GetScrollY();

    if (this->scrollToBottom)
    {
        // Growing content does not move the scroll position, so being below where we left it last
        // frame can
        // only mean the user scrolled up. The expected position is clamped because trimming the log
        // can shrink it
        const float expected = this->lastMaxScroll < maxScroll ? this->lastMaxScroll : maxScroll;
        if (scrollTop < expected - 1.0f)
            this->scrollToBottom = false;
    }
    else if (scrollTop >= maxScroll - 1.0f)
    {
        // The user scrolled back down to the bottom, start following the log again
        this->scrollToBottom = true;
    }
    this->lastMaxScroll = maxScroll;

    // A row is as tall as its wrapped message makes it, and the list clipper only knows how to lay
    // out rows of
    // equal height, so the virtualization is done by hand: the dummy gives the region the scroll
    // range of the
    // whole list, and only the rows that intersect the viewport are submitted
    this->BuildRows(entries, ImGui::GetContentRegionAvail().x);
    ImGui::Dummy(ImVec2(0, this->totalRowHeight));

    const IndexT numRows = (IndexT)this->rowTops.Size() - 1;
    if (numRows > 0)
    {
        // The first row that has not ended above the viewport...
        IndexT first = 0;
        {
            IndexT lo = 0, hi = numRows - 1;
            while (lo < hi)
            {
                IndexT mid = (lo + hi) / 2;
                if (this->rowTops[mid + 1] > scrollTop)
                    hi = mid;
                else
                    lo = mid + 1;
            }
            first = lo;
        }
        // ...and the last one that has not started below it
        IndexT last = numRows - 1;
        {
            IndexT lo = first, hi = numRows - 1;
            while (lo < hi)
            {
                IndexT mid = (lo + hi + 1) / 2;
                if (this->rowTops[mid] < scrollTop + ImGui::GetWindowHeight())
                    lo = mid;
                else
                    hi = mid - 1;
            }
            last = lo;
        }

        for (IndexT i = first; i <= last; i++)
        {
            // The rows are placed at their measured tops, so a row measured wrong shifts itself
            // rather than
            // everything drawn under it
            ImGui::SetCursorPosY(this->rowTops[i]);

            const LogEntry& entry = entries[this->visible[i]];

            ImGui::PushStyleColor(ImGuiCol_Text, this->FilterColor(entry.type));
            ImGui::TextUnformatted(entry.timestamp.AsCharPtr());
            ImGui::SameLine();
            ImGui::TextUnformatted(this->EntryTag(entry.type));
            ImGui::SameLine();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
            ImGui::TextUnformatted(entry.msg.AsCharPtr());
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
    }

    // The scroll target is applied on the next frame and clamped to the scroll range then.
    // SetScrollHereY is not
    // usable here: it aims at the last submitted item, which after a frame where the log grew is
    // the bottom of
    // the old viewport, and the scroll would pin there instead of following. Asking for the scroll
    // position of
    // the content bottom lands exactly there, since the range covers the whole list
    if (this->scrollToBottom)
        ImGui::SetScrollY(this->totalRowHeight);

    ImGui::EndChild();

    log->Leave();
    
    ImGui::Separator();

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_CallbackHistory |
        ImGuiInputTextFlags_CallbackCompletion;

    ImGui::PushItemWidth(avail);
    if (this->inputFocusRequested)
    {
        ImGui::SetKeyboardFocusHere();
        this->inputFocusRequested = false;
    }
    if (ImGui::InputTextWithHint("##command", "> Enter command", this->inputBuffer, MaxInputLength, flags, &ConsolePanel::InputCallback, this))
    {
        if (this->inputBuffer[0] != 0)
            this->Execute(Util::String(this->inputBuffer));
        this->inputBuffer[0] = 0;

        // Enter drops the field, which is not what a console wants: the command has run and the
        // field should be
        // ready for the next one without the user having to click it again. The request is applied
        // at the end of
        // the frame, and the widget reads its text from the buffer when it is activated, so the
        // field comes back
        // empty rather than with the command that was just run
        ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::PopItemWidth();

}

//------------------------------------------------------------------------------
/**
*/
void
ConsolePanel::Execute(const Util::String& command)
{
    n_assert(!command.IsEmpty());

    this->core->Log(LogMessageType::N_INPUT, command);
    this->core->ExecuteCommand(command);

    this->previousCommands.Append(command);
    this->previousCommandIndex = -1;
}

//------------------------------------------------------------------------------
/**
    The command is put in front of the user rather than run, because what it still lacks is a path
    only they know. The panel is opened with it, because a command sitting in a closed panel is not
    something anyone sees.
*/
void
ConsolePanel::SeedCommand(const Util::String& command)
{
    command.CopyToBuffer(this->inputBuffer, MaxInputLength);
    this->inputFocusRequested = true;
    this->SetOpen(true);
}

//------------------------------------------------------------------------------
/**
    The callback runs inside the input widget, so the buffer is written through the callback data
    rather than through the panel.
*/
int
ConsolePanel::InputCallback(ImGuiInputTextCallbackData* data)
{
    ConsolePanel* panel = static_cast<ConsolePanel*>(data->UserData);

    if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
    {
        const Util::String& command = panel->RecallCommand(data->EventKey == ImGuiKey_UpArrow ? -1 : 1);
        data->DeleteChars(0, data->BufTextLen);
        if (!command.IsEmpty())
            data->InsertChars(0, command.AsCharPtr());
    }

    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
ConsolePanel::CommandHelp(const Util::Array<Util::String>& args)
{
    (void)args;

    this->core->Log(LogMessageType::N_SYSTEM, "help             list the console commands");
    this->core->Log(LogMessageType::N_SYSTEM, "clear            clear the log");
    this->core->Log(LogMessageType::N_SYSTEM, "history          list the commands that were run this session");

    const EditorCommandSystem& commands = this->core->GetCommands();
    for (IndexT i = 0; i < commands.GetCommands().Size(); i++)
    {
        const EditorCommand& command = commands.GetCommands()[i];
        if (command.name == "help")
            continue;

        Util::String line;
        line.Format("%-15s   %s", command.name.AsCharPtr(), command.description.AsCharPtr());
        this->core->Log(LogMessageType::N_SYSTEM, line);
    }

    this->core->Log(LogMessageType::N_SYSTEM, "<cvar>           print the value of a console variable");
    this->core->Log(LogMessageType::N_SYSTEM, "<cvar> <value>   set a console variable, for example 'cl_debug 1'");
    this->core->Log(LogMessageType::N_SYSTEM, "A word in quotes keeps its spaces, so 'select \"Entity 1\"' is one name");
    this->core->Log(LogMessageType::N_SYSTEM, "Up and Down arrows walk through the command history");
}

//------------------------------------------------------------------------------
/**
*/
void
ConsolePanel::CommandClear(const Util::Array<Util::String>& args)
{
    (void)args;

    EditorLog* log = this->core->GetLog();
    if (log != nullptr)
        log->Clear();

    this->scrollToBottom = true;
    this->lastMaxScroll = 0.0f;
    this->visibleDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ConsolePanel::CommandHistory(const Util::Array<Util::String>& args)
{
    (void)args;

    if (this->previousCommands.IsEmpty())
    {
        this->core->Log(LogMessageType::N_SYSTEM, "No commands have been run yet");
        return;
    }

    for (IndexT i = 0; i < this->previousCommands.Size(); i++)
        this->core->Log(LogMessageType::N_MESSAGE, this->previousCommands[i]);
}

//------------------------------------------------------------------------------
/**
*/
Util::String
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
void
ConsolePanel::CommandListCVars(const Util::Array<Util::String>& args)
{
    (void)args;

    for (Core::CVar* cVar = Core::CVarsBegin(); cVar != Core::CVarsEnd(); cVar = Core::CVarNext(cVar))
    {
        Util::String msg;
        msg.Format("%s = %s", Core::CVarGetName(cVar), CVarValueAsString(cVar).AsCharPtr());
        this->core->Log(LogMessageType::N_MESSAGE, msg);
    }
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
ConsolePanel::RecallCommand(int direction)
{
    if (this->previousCommands.IsEmpty())
        return this->emptyCommand;

    const IndexT numCommands = (IndexT)this->previousCommands.Size();
    if (direction < 0)
    {
        if (this->previousCommandIndex == -1)
            this->previousCommandIndex = numCommands - 1;
        else if (this->previousCommandIndex > 0)
            this->previousCommandIndex--;
    }
    else if (this->previousCommandIndex != -1)
    {
        this->previousCommandIndex++;
        if (this->previousCommandIndex >= numCommands)
            this->previousCommandIndex = -1;
    }

    if (this->previousCommandIndex >= 0)
        return this->previousCommands[this->previousCommandIndex];
    return this->emptyCommand;
}

//------------------------------------------------------------------------------
/**
    The color a log entry is drawn with. The log types have no style slot of their own, so they are
    palette entries rather than style colors
*/
ImVec4
ConsolePanel::FilterColor(LogMessageType type) const
{
    switch (type)
    {
    case LogMessageType::N_WARNING:
        return ColorFromHex(PaletteWarning);
    case LogMessageType::N_ERROR:
    case LogMessageType::N_EXCEPTION:
        return ColorFromHex(PaletteError);
    case LogMessageType::N_SYSTEM:
        return ColorFromHex(PaletteMuted);
    case LogMessageType::N_INPUT:
        return ColorFromHex(PaletteAccent);
    default:
        break;
    }
    return ImGui::GetStyle().Colors[ImGuiCol_Text];
}

//------------------------------------------------------------------------------
/**
*/
const char*
ConsolePanel::EntryTag(LogMessageType type) const
{
    return EditorLog::Tag(type);
}

//------------------------------------------------------------------------------
/**
*/
int
ConsolePanel::FilterType(const LogFilter& filter) const
{
    switch (filter)
    {
    case LogFilter::FilterMessage:
        return (int)LogMessageType::N_MESSAGE;
    case LogFilter::FilterWarning:
        return (int)LogMessageType::N_WARNING;
    case LogFilter::FilterError:
        return (int)LogMessageType::N_ERROR;
    default:
        break;
    }
    return -1;
}

//------------------------------------------------------------------------------
/**
*/
bool
ConsolePanel::MatchesFilter(const LogEntry& entry) const
{
    if (this->activeFilter == LogFilter::FilterAll)
        return true;
    if (entry.type == LogMessageType::N_INPUT)
        return true;
    return (int)entry.type == this->FilterType(this->activeFilter);
}

} // namespace Editor
