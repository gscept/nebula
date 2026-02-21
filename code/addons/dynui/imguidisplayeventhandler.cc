//------------------------------------------------------------------------------
//  imguidisplayeventhandler.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "imguidisplayeventhandler.h"
#include "imgui.h"
namespace Dynui
{
__ImplementClass(Dynui::ImguiDisplayEventHandler, 'IDEH', CoreGraphics::DisplayEventHandler);

using namespace Input;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
bool
ImguiDisplayEventHandler::HandleEvent(const DisplayEvent& displayEvent)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (displayEvent.GetEventCode())
    {
        case DisplayEvent::CloseRequested:
        {
            CoreGraphics::WindowId wnd = displayEvent.GetWindowId();
            ImGuiID* id = static_cast<ImGuiID*>(CoreGraphics::WindowGetUserData(wnd));
            if (wnd.id != 0)
            {
                ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
                for (uint i = 0; i < platform_io.Viewports.Size; i++)
                {
                    ImGuiViewport* viewport = platform_io.Viewports[i];
                    if (viewport->ID == *id)
                    {
                        viewport->PlatformRequestClose = true;
                        return true;
                    }
                }
            }
            break;
        }
        case DisplayEvent::SetFocus:
        {
            io.AddFocusEvent(true);
            break;
        }
        case DisplayEvent::WindowResized:
        {
            CoreGraphics::WindowId wnd = displayEvent.GetWindowId();
            ImGuiID* id = static_cast<ImGuiID*>(CoreGraphics::WindowGetUserData(wnd));
            if (wnd.id != 0)
            {
                ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
                for (uint i = 0; i < platform_io.Viewports.Size; i++)
                {
                    ImGuiViewport* viewport = platform_io.Viewports[i];
                    if (viewport->ID == *id)
                    {
                        viewport->PlatformRequestResize = true;
                        return true;
                    }
                }
            }
            break;
        }
        case DisplayEvent::WindowMoved:
        {
            CoreGraphics::WindowId wnd = displayEvent.GetWindowId();
            ImGuiID* id = static_cast<ImGuiID*>(CoreGraphics::WindowGetUserData(wnd));
            if (wnd.id != 0)
            {
                ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
                for (uint i = 0; i < platform_io.Viewports.Size; i++)
                {
                    ImGuiViewport* viewport = platform_io.Viewports[i];
                    if (viewport->ID == *id)
                    {
                        viewport->PlatformRequestMove = true;
                        return true;
                    }
                }
            }
            break;
        }
        case DisplayEvent::MouseMove:
        {
            Math::int2 pos = CoreGraphics::WindowGetPosition(displayEvent.GetWindowId());
            io.AddMousePosEvent(displayEvent.GetAbsMousePos().x + pos.x, displayEvent.GetAbsMousePos().y + pos.y);
            return io.WantCaptureMouse;
        }
        case DisplayEvent::MouseWheelForward:
            io.AddMouseWheelEvent(0, 1);
            return io.WantCaptureMouse;
        case DisplayEvent::MouseWheelBackward:
            io.AddMouseWheelEvent(0, -1);
            return io.WantCaptureMouse;
        default: break;
    }
    return false;
}

} // namespace GLFW
