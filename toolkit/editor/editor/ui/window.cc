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
    open(true)
{
}

//------------------------------------------------------------------------------
/**
*/
BaseWindow::BaseWindow(Util::String name) :
    name(name),
    open(false)
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
const
Util::String & BaseWindow::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::SetName(const char * name)
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
BaseWindow::SetCategory(const char * category)
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
BaseWindow::Run()
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
        ImGuiWindowSettings* imWindowSettings = ImGui::FindWindowSettings(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
        if (imWindowSettings != nullptr)
        {
            pos.x = imWindowSettings->Pos.x;
            pos.y = imWindowSettings->Pos.y;
        }
        else
        {
            // Nothing found, return default
            pos = { 0, 0 };
        }
    }
    
    return { pos.x, pos.y };
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
        ImGuiWindowSettings* imWindowSettings = ImGui::FindWindowSettings(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
        if (imWindowSettings != nullptr)
        {
            size = imWindowSettings->Size;
        }
        else
        {
            // Nothing found, return default
            size = { 0, 0 };
        }
    }


    return { (float)size.x, (float)size.y };
}

//------------------------------------------------------------------------------
/**
*/
void
BaseWindow::SetPosition(const Math::vec2 & pos)
{
    ImGuiWindow* imwindow = ImGui::FindWindowByName(this->name.AsCharPtr());
    if (imwindow != nullptr)
    {
        imwindow->Pos = ImVec2(pos.x, pos.y);
        return;
    }

    // check if we've got any settings for this window.
    ImGuiWindowSettings* imWindowSettings = ImGui::FindWindowSettings(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
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
BaseWindow::SetSize(const Math::vec2 & size)
{
    ImGuiWindow* imwindow = ImGui::FindWindowByName(this->name.AsCharPtr());
    if (imwindow != nullptr)
    {
        imwindow->Size = ImVec2(size.x, size.y);
        return;
    }

    // check if we've got any settings for this window.
    ImGuiWindowSettings* imWindowSettings = ImGui::FindWindowSettings(ImHashStr(this->name.AsCharPtr(), this->name.Length()));
    if (imWindowSettings != nullptr)
    {
        imWindowSettings->Size = ImVec2ih(size.x, size.y);
        return;
    }

    // No window or settings found, create new settings for the next time the window is opened.
    imWindowSettings = ImGui::CreateNewWindowSettings(this->name.AsCharPtr());
    imWindowSettings->Size = ImVec2ih(size.x, size.y);
}

} // namespace Presentation
