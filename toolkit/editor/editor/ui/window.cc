//------------------------------------------------------------------------------
//  window.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "window.h"
#include "imgui_internal.h"

namespace Presentation
{

__ImplementClass(Presentation::BaseWindow, 'bWnd', Core::RefCounted)

//------------------------------------------------------------------------------
/**
*/
BaseWindow::BaseWindow() : 
    additionalFlags((ImGuiWindowFlags_)0),
    name("UNNAMED WINDOW"),
    open(true),
    editCounter(0)
{
}

//------------------------------------------------------------------------------
/**
*/
BaseWindow::BaseWindow(Util::String name) : 
    name(name),
    open(false),
    editCounter(0)
{
}

//------------------------------------------------------------------------------
/**
*/
BaseWindow::~BaseWindow()
{
}

//------------------------------------------------------------------------------
/**
*/
const Util::String
BaseWindow::GetName() const
{
    return FormatName(this->name, this->editCounter);
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::SetName(const char* name)
{
    this->name = name;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
BaseWindow::GetCategory() const
{
    return this->category;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::SetCategory(const char* category)
{
    this->category = category;
}

//------------------------------------------------------------------------------
/**
*/
bool&
BaseWindow::Open()
{
    return this->open;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::Run(SaveMode save)
{
    return;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::Update()
{
    return;
}

//------------------------------------------------------------------------------
/**
*/
const ImGuiWindowFlags_&
BaseWindow::GetAdditionalFlags() const
{
    return this->additionalFlags;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2
BaseWindow::GetWindowPadding() const
{
    if (this->usesCustomWindowPadding)
    {
        return this->windowPadding;
    }
    else
    {
        ImGuiStyle& style = ImGui::GetStyle();
        return {style.WindowPadding.x, style.WindowPadding.y};
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::SetWindowPadding(const Math::vec2& padding)
{
    this->usesCustomWindowPadding = true;
    this->windowPadding = padding;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2
BaseWindow::GetPosition() const
{
    ImVec2 pos;
    ImGuiWindow* imwindow = ImGui::FindWindowByName(this->name.AsCharPtr());
    if (imwindow != nullptr)
    {
        pos = imwindow->Pos;
    }
    else
    {
        // check if we've got any settings for this window.
        ImGuiWindowSettings* imWindowSettings =
            ImGui::FindWindowSettingsByID(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
        if (imWindowSettings != nullptr)
        {
            pos.x = imWindowSettings->Pos.x;
            pos.y = imWindowSettings->Pos.y;
        }
        else
        {
            // Nothing found, return default
            pos = {0, 0};
        }
    }

    return {pos.x, pos.y};
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2
BaseWindow::GetSize() const
{
    ImVec2ih size;
    ImGuiWindow* imwindow = ImGui::FindWindowByName(this->name.AsCharPtr());
    if (imwindow != nullptr)
    {
        size.x = (short)imwindow->Size.x;
        size.y = (short)imwindow->Size.y;
    }
    else
    {
        // check if we've got any settings for this window.
        ImGuiWindowSettings* imWindowSettings =
            ImGui::FindWindowSettingsByID(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
        if (imWindowSettings != nullptr)
        {
            size = imWindowSettings->Size;
        }
        else
        {
            // Nothing found, return default
            size = {0, 0};
        }
    }

    return {(float)size.x, (float)size.y};
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::SetPosition(const Math::vec2& pos)
{
    ImGuiWindow* imwindow = ImGui::FindWindowByName(this->name.AsCharPtr());
    if (imwindow != nullptr)
    {
        imwindow->Pos = ImVec2(pos.x, pos.y);
        return;
    }

    // check if we've got any settings for this window.
    ImGuiWindowSettings* imWindowSettings = ImGui::FindWindowSettingsByID(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
    if (imWindowSettings != nullptr)
    {
        imWindowSettings->Pos = ImVec2ih(pos.x, pos.y);
        return;
    }

    // No window or settings found, create new settings for the next time the window is opened.
    imWindowSettings = ImGui::CreateNewWindowSettings(this->name.AsCharPtr());
    imWindowSettings->Pos = ImVec2ih(pos.x, pos.y);
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::SetSize(const Math::vec2& size)
{
    ImGuiWindow* imwindow = ImGui::FindWindowByName(this->name.AsCharPtr());
    if (imwindow != nullptr)
    {
        imwindow->Size = ImVec2(size.x, size.y);
        return;
    }

    // check if we've got any settings for this window.
    ImGuiWindowSettings* imWindowSettings = ImGui::FindWindowSettingsByID(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
    if (imWindowSettings != nullptr)
    {
        imWindowSettings->Size = ImVec2ih(size.x, size.y);
        return;
    }

    // No window or settings found, create new settings for the next time the window is opened.
    imWindowSettings = ImGui::CreateNewWindowSettings(this->name.AsCharPtr());
    imWindowSettings->Size = ImVec2ih(size.x, size.y);
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseWindow::Edit()
{
    this->editCounter++;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseWindow::Unedit(int count)
{
    this->editCounter -= count;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseWindow::Save()
{
    this->editCounter = 0;
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
BaseWindow::FormatName(const Util::String& name, int editCounter)
{
    if (editCounter != 0)
    {
        return Util::Format("%s*###%s", name.AsCharPtr(), name.AsCharPtr());
    }
    else
    {
        return Util::Format("%s###%s", name.AsCharPtr(), name.AsCharPtr());
    }
}

} // namespace Presentation
