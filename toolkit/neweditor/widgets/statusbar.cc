//------------------------------------------------------------------------------
//  @file statusbar.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/widgets/statusbar.h"
#include "basegamefeature/managers/timemanager.h"
#include "neweditor/api/shell.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/core/editorpanel.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::StatusBar, 'SBA', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
StatusBar::StatusBar() :
    core(nullptr),
    showBarCVar(nullptr),
    expires(0.0),
    averageFrameTime(0.0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StatusBar::~StatusBar()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The status bar is not a window a user can move or close, so it is always part of the editor and
    its cvar is created here rather than by a feature unit.
*/
bool
StatusBar::Setup(Shell* core)
{
    this->core = core;

    Core::CVarCreate(Core::CVarType::CVar_Int, "ed_show_status_bar", "1",
        "Show the status bar at the bottom of the editor");

    this->showBarCVar = Core::CVarGet("ed_show_status_bar");
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
StatusBar::SetStatus(const Util::String& msg, float duration)
{
    this->message = msg;

    if (duration > 0.0f)
    {
        const Game::TimeSource* time = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
        this->expires = time->time + duration;
    }
    else
    {
        this->expires = 0.0;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
StatusBar::ClearStatus()
{
    this->message = Util::String();
    this->expires = 0.0;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
StatusBar::Message() const
{
    return this->message;
}

//------------------------------------------------------------------------------
/**
*/
bool
StatusBar::IsVisible() const
{
    return this->showBarCVar != nullptr && Core::CVarReadInt(this->showBarCVar) != 0;
}

//------------------------------------------------------------------------------
/**
    One line of text with the padding around it. The bar has no title bar, so this is what it takes.
*/
float
StatusBar::Height() const
{
    if (!this->IsVisible())
        return 0.0f;

    return ImGui::GetFrameHeight();
}

//------------------------------------------------------------------------------
/**
    The bar is placed at the bottom of the viewport rather than docked into it, which is what keeps
    it where it is whatever the layout looks like.
*/
void
StatusBar::Draw()
{
    if (!this->IsVisible())
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, 5);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ColorFromHex(PalettePanel));

    // A timed message is expired here rather than on a timer, because the bar only advances when it
    // is drawn
    if (!this->message.IsEmpty() && this->expires > 0.0)
    {
        const Game::TimeSource* time = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
        if (time->time >= this->expires)
            this->message = Util::String();
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = this->Height();

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - height), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, height), ImGuiCond_Always);
    ImGui::SetNextWindowViewport(viewport->ID);

    // A bar the user cannot move, resize, collapse or dock into, and that is not in the layout
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("Status bar", nullptr, flags))
    {
        if (!this->message.IsEmpty())
        {
            ImGui::TextDisabled(this->message.AsCharPtr());
            ImGui::SameLine();
        }

        this->DrawNumbers();
    }
    ImGui::End();


    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

//------------------------------------------------------------------------------
/**
    The numbers. They are read when the bar draws, which is the immediate mode version of dirtying a
    widget.
*/
void
StatusBar::DrawNumbers()
{
    const Game::TimeSource* time = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    const Timing::Time dt = time->frameTime;
    if (dt > 0.0)
        this->averageFrameTime = this->averageFrameTime > 0.0 ? this->averageFrameTime * 0.9 + dt * 0.1 : dt;

    Util::String text;
    if (this->averageFrameTime > 0.0)
        text.Format("%.0f fps", (float)(1.0 / this->averageFrameTime));
    else
        text = "0 fps";

    // The bar reads the frame time itself, because that is the one number the editor has. What it
    // cannot read is
    // what a feature put on it: the draw calls are the renderer's and the render resolution is the
    // size of the
    // target a feature renders into, and neither is something the editor knows a type for
    if (this->core != nullptr)
    {
        const Util::Array<EditorStatusNumbers>& numbers = EditorFeatureRegistry::GetStatusNumbers();
        for (IndexT i = 0; i < numbers.Size(); i++)
        {
            Util::String part;
            numbers[i].append(part);
            if (part.IsEmpty())
                continue;

            text.Append("   ");
            text.Append(part);
        }
    }

    // Right aligned on the same line as the message
    float off = ImGui::GetWindowWidth() - ImGui::CalcTextSize(text.AsCharPtr()).x - ImGui::GetStyle().WindowPadding.y;
    if (off > 0.0f)
        ImGui::SetCursorPosX(off);

    ImGui::TextUnformatted(text.AsCharPtr());
}

} // namespace Editor
