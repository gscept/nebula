#pragma once
//------------------------------------------------------------------------------
/**
    @file imguidetach.h

    Keeps a window that is dragged out of the main window in the main window until it is let go,
    and only then gives it its own OS window. Only needs ImGui, so code that does not depend on the
    renderer can call it before its Begin(). The focus half runs in ImguiContext::NewFrame.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "imgui.h"
#include "imgui_internal.h"

namespace Dynui::ImguiDetach
{

//------------------------------------------------------------------------------
/**
    The window being dragged out of the main window, where it is to be let go, and the window that
    was let go outside the main window and takes the focus once its OS window is shown
*/
struct DetachStateType
{
    ImGuiID dragged = 0;
    ImVec2 dropPos;
    ImGuiID focusOnShow = 0;
};
inline DetachStateType State;

//------------------------------------------------------------------------------
/**
    By default ImGui gives a window its own OS window as soon as it is dragged past the edge of the
    main window, while the mouse button is still held. On X11 the main window holds the pointer for as
    long as the button is down, and the new window has to be created, mapped and moved every frame
    against that, which is what makes dragging out janky.

    Instead the window stays in the main window for as long as it is dragged: outside of it it is
    simply not seen. Its place is set each frame from the mouse, because ImGui would otherwise keep a
    window in its viewport and it would stop at the edge. Once let go ImGui finds it outside the main
    window and gives it its OS window there, once, with no button held.
*/
inline void
DeferWindowDetach(const char* name)
{
    ImGuiContext& g = *GImGui;
    if ((g.ConfigFlagsCurrFrame & ImGuiConfigFlags_ViewportsEnable) == 0)
    {
        return;
    }

    ImGuiWindow* window = ImGui::FindWindowByName(name);
    if (window == nullptr)
    {
        return;
    }

    // Let go: the window goes where it was dropped, and ImGui makes its OS window there
    if (State.dragged == window->ID && g.MovingWindow != window)
    {
        State.dragged = 0;

        // Dropped onto a dock, which places it
        if (window->DockNode != nullptr)
        {
            return;
        }

        ImGui::SetNextWindowPos(State.dropPos, ImGuiCond_Always);
        // Written out, because the math operators are not there for every file that includes ImGui
        const ImGuiViewport* main = ImGui::GetMainViewport();
        const bool inside = State.dropPos.x >= main->Pos.x && State.dropPos.y >= main->Pos.y &&
            State.dropPos.x + window->Size.x <= main->Pos.x + main->Size.x &&
            State.dropPos.y + window->Size.y <= main->Pos.y + main->Size.y;
        if (!inside)
        {
            State.focusOnShow = window->ID;
        }
        return;
    }

    // Only a window that is dragged on its own, and is still in the main window. A window that has its
    // own OS window already moves with it, and a docked window moves with its dock
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    if (g.MovingWindow != window || window->Viewport != mainViewport || window->ViewportOwned || window->DockIsActive)
    {
        return;
    }
    if (!ImGui::IsMousePosValid() || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        return;
    }

    // The same place ImGui moves the window to, but set through the API so it is not kept inside
    State.dragged = window->ID;
    State.dropPos = ImVec2(g.IO.MousePos.x - g.ActiveIdClickOffset.x, g.IO.MousePos.y - g.ActiveIdClickOffset.y);
    ImGui::SetNextWindowPos(State.dropPos, ImGuiCond_Always);
    ImGui::SetNextWindowViewport(mainViewport->ID);
}


} // namespace Dynui::ImguiDetach
