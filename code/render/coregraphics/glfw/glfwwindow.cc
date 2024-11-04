//------------------------------------------------------------------------------
// glfwwindow.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/config.h"
#include "glfwwindow.h"
#include "glfwdisplaydevice.h"
#include "input/key.h"
#include "coregraphics/displayevent.h"
#include "coregraphics/displaydevice.h"
#include "math/scalar.h"
#include "coregraphics/swapchain.h"

#if __VULKAN__
#include "coregraphics/vk/vkgraphicsdevice.h"
#include "coregraphics/vk/vktypes.h"
#endif

namespace CoreGraphics
{
WindowId CurrentWindow;
GLFWWindowAllocatorType glfwWindowAllocator(0x00FFFFFF);

}

using namespace CoreGraphics;
using namespace Math;
namespace GLFW
{

//------------------------------------------------------------------------------
/**
*/
    /*
void
GLFWWindow::Reopen()
{
    // just ignore full screen if this window is embedded
    if (!this->embedded)
    {
        // if we toggle full screen, select monitor (just selects primary for the moment) and apply full screen
        if (this->fullscreen)
        {
            this->ApplyFullscreen();
        }
        else
        {
            // if not, set the window state and detach from the monitor
            glfwSetWindowMonitor(this->window, NULL, this->displayMode.GetXPos(), this->displayMode.GetYPos(), this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->displayMode.GetRefreshRate());
        }
    }

    // only move window if not fullscreen
    if (!this->fullscreen)
    {
        // update window with new size and position
        glfwSetWindowPos(this->window, this->displayMode.GetXPos(), this->displayMode.GetYPos());
        glfwSetWindowSize(this->window, this->displayMode.GetWidth(), this->displayMode.GetHeight());
    }

    // post event
    GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowReopen, this->windowId));

    // open window again
    WindowBase::Reopen();
}
*/

//------------------------------------------------------------------------------
/**
*/
void
KeyFunc(const CoreGraphics::WindowId& id, int key, int scancode, int action, int mods)
{
    Input::Key::Code keyCode = GLFWDisplayDevice::TranslateKeyCode(key);
    DisplayEvent::Code evtype;
    switch (action)
    {
    case GLFW_REPEAT:
    case GLFW_PRESS:
        evtype = DisplayEvent::KeyDown;
        break;
    case GLFW_RELEASE:
        evtype = DisplayEvent::KeyUp;
        break;
    default:
        return;
    }
    if (Input::Key::InvalidKey != keyCode)
    {
        GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(evtype, keyCode));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CharFunc(const CoreGraphics::WindowId& id, unsigned int key)
{
    GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::Character, (Input::Char)key));
}

//------------------------------------------------------------------------------
/**
*/
void
MouseButtonFunc(const CoreGraphics::WindowId& id, int button, int action, int mods)
{
    DisplayEvent::Code act = action == GLFW_PRESS ? DisplayEvent::MouseButtonDown : DisplayEvent::MouseButtonUp;
    Input::MouseButton::Code but;
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
        but = Input::MouseButton::LeftButton;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        but = Input::MouseButton::RightButton;
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        but = Input::MouseButton::MiddleButton;
        break;
    default:
        return;
    }

    GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    const CoreGraphics::DisplayMode& mode = glfwWindowAllocator.Get<GLFW_DisplayMode>(id.id);
    double x, y;
    glfwGetCursorPos(wnd, &x, &y);

    vec2 pos;
    pos.set((float)x / float(mode.GetWidth()), (float)y / float(mode.GetHeight()));
    GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(act, but, vec2((float)x, (float)y), pos));
}

//------------------------------------------------------------------------------
/**
*/
void
MouseFunc(const CoreGraphics::WindowId& id, double xpos, double ypos)
{
    const CoreGraphics::DisplayMode& mode = glfwWindowAllocator.Get<GLFW_DisplayMode>(id.id);
    vec2 absMousePos((float)xpos, (float)ypos);
    vec2 pos;
    pos.set((float)xpos / float(mode.GetWidth()), (float)ypos / float(mode.GetHeight()));
    GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseMove, absMousePos, pos));
}

//------------------------------------------------------------------------------
/**
*/
void
ScrollFunc(const CoreGraphics::WindowId& id, double xs, double ys)
{
    if (ys != 0.0)
    {
        GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(ys > 0.0f ? DisplayEvent::MouseWheelForward : DisplayEvent::MouseWheelBackward));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CloseFunc(const CoreGraphics::WindowId& id)
{
    GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::CloseRequested, id));
}

//------------------------------------------------------------------------------
/**
*/
void
FocusFunc(const CoreGraphics::WindowId& id, int focus)
{
    if (focus)
    {
        GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::SetFocus, id));
    }
    else
    {
        GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::KillFocus, id));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EnableCallbacks(const CoreGraphics::WindowId & id)
{
    GLFWwindow* window = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    glfwSetKeyCallback(window, [](GLFWwindow * window, int key, int scancode, int action, int mods)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        KeyFunc(*id, key, scancode, action, mods);
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow * window, int button, int action, int mods)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        MouseButtonFunc(*id, button, action, mods);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow * window, double xpos, double ypos)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        MouseFunc(*id, xpos, ypos);
    });
    glfwSetWindowCloseCallback(window, [](GLFWwindow * window)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        CloseFunc(*id);
    });
    glfwSetWindowFocusCallback(window, [](GLFWwindow * window, int focus)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        FocusFunc(*id, focus);
    });
    glfwSetWindowSizeCallback(window, [](GLFWwindow * window, int width, int height)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        ResizeFunc(*id, width, height);
    });
    glfwSetScrollCallback(window, [](GLFWwindow * window, double xs, double ys)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        ScrollFunc(*id, xs, ys);
    });
    glfwSetCharCallback(window, [](GLFWwindow * window, unsigned int key)
    {
        CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
        CharFunc(*id, key);
    });
    glfwSetDropCallback(window, [](GLFWwindow* window, int path_count, const char* paths[])
    {

    });
}

//------------------------------------------------------------------------------
/**
*/
void
DisableCallbacks(const CoreGraphics::WindowId & id)
{
    GLFWwindow* window = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    glfwSetKeyCallback(window, nullptr);
    glfwSetMouseButtonCallback(window, nullptr);
    glfwSetCursorPosCallback(window, nullptr);
    glfwSetWindowCloseCallback(window, nullptr);
    glfwSetWindowFocusCallback(window, nullptr);
    glfwSetWindowSizeCallback(window, nullptr);
    glfwSetScrollCallback(window, nullptr);
    glfwSetCharCallback(window, nullptr);
    glfwSetDropCallback(window, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
const WindowId
InternalSetupFunction(const WindowCreateInfo& info, const Util::Blob& windowData, bool embed)
{
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, info.aa == AntiAliasQuality::Low ? 2:
                                 info.aa == AntiAliasQuality::Medium ? 4:
                                info.aa == AntiAliasQuality::High ? 8 : 0);


    Ids::Id32 windowId = glfwWindowAllocator.Alloc();
    WindowId id = windowId;
    glfwWindowAllocator.Set<GLFW_SetupInfo>(windowId, info);
    glfwWindowAllocator.Set<GLFW_ResizeInfo>(windowId, { .newWidth = 0, .newHeight = 0, .done = true, .vsync = info.vsync });

    GLFWmonitor* monitor = GLFWDisplayDevice::Instance()->GetMonitor(Adapter::Code::Primary);
    n_assert(monitor);
    
    // get original window, if this is the first window, then the parent window will simply be nullptr
    GLFWwindow* wnd = nullptr;
    const CoreGraphics::WindowId origWindow = CoreGraphics::DisplayDevice::Instance()->GetMainWindow();
    if (origWindow.index != Ids::InvalidId32) wnd = glfwWindowAllocator.Get<GLFW_Window>(origWindow.id);

    CoreGraphics::DisplayMode& mode = glfwWindowAllocator.Get<GLFW_DisplayMode>(id.id);
    mode = info.mode;

#if __VULKAN__
    // if Vulkan, context is created and managed by render device
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif
    // scale window according to platform dpi
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    float xScale, yScale;
    glfwGetMonitorContentScale(monitor, &xScale, &yScale);
    float contentScale = Math::max(xScale, yScale);
    mode.SetWidth(info.mode.GetWidth() * contentScale);
    mode.SetHeight(info.mode.GetHeight() * contentScale);
    mode.SetContentScale(contentScale);

    // set user pointer to this window
    WindowId* ptr = new WindowId;
    *ptr = id;
    if (embed)
    {
        n_error("FIXME, createwindowfromalien not implemented yet");
#if 0
        // create window using our Qt window as child
        wnd = glfwCreateWindowFromAlien(windowData.GetPtr(), wnd);
        glfwMakeContextCurrent(wnd);

        // get actual window size
        int height, width;
        glfwGetWindowSize(wnd, &width, &height);

        // update display mode
        mode.SetWidth(width);
        mode.SetHeight(height);
        mode.SetAspectRatio(width / float(height));

        glfwSetWindowUserPointer(wnd, ptr);
#endif
    }
    else
    {
        //FIXME resize is broken currently, forcibly disabling it
        glfwWindowHint(GLFW_RESIZABLE, info.resizable ? GL_TRUE : GL_FALSE);
        glfwWindowHint(GLFW_DECORATED, info.decorated ? GL_TRUE : GL_FALSE);

        // create window
        wnd = glfwCreateWindow(info.mode.GetWidth(), info.mode.GetHeight(), info.title.Value(), nullptr, wnd);

        if (!info.fullscreen)   glfwSetWindowPos(wnd, info.mode.GetXPos(), info.mode.GetYPos());
        else                    WindowApplyFullscreen(id, Adapter::Primary, true);

        // set user pointer to this window
        glfwSetWindowUserPointer(wnd, ptr);
        glfwSetWindowTitle(wnd, info.title.Value());
    }

    glfwSwapInterval(info.vsync ? CoreGraphics::GetNumBufferedFrames() : 0);

    CoreGraphics::SwapchainCreateInfo swapCreate;
    swapCreate.displayMode = mode;
    swapCreate.vsync = info.vsync;
    swapCreate.window = wnd;
    SwapchainId swapchain = CoreGraphics::CreateSwapchain(swapCreate);

    glfwWindowAllocator.Set<GLFW_Window>(windowId, wnd);
    glfwWindowAllocator.Set<GLFW_Swapchain>(windowId, swapchain);

    // notify window is opened
    GLFW::GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowOpen, id));

    // enable callbacks
    GLFW::EnableCallbacks(id);

    DisplayDevice::Instance()->MakeWindowCurrent(id);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
ResizeFunc(const CoreGraphics::WindowId& id, int width, int height)
{
    ResizeInfo& info = glfwWindowAllocator.Get<GLFW_ResizeInfo>(id.id);

    // only resize if size is not 0
    if (width != 0 && height != 0)
    {
        info.newWidth = width;
        info.newHeight = height;
        info.done = false;
    }
}


} // namespace GLFW

namespace CoreGraphics
{

#ifdef __VULKAN__
using namespace Vulkan;
#endif


//------------------------------------------------------------------------------
/**
*/
const WindowId
CreateWindow(const WindowCreateInfo& info)
{
    return GLFW::InternalSetupFunction(info, nullptr, false);
}

//------------------------------------------------------------------------------
/**
*/
const WindowId
EmbedWindow(const Util::Blob& windowData)
{
    return GLFW::InternalSetupFunction(WindowCreateInfo(), windowData, true);
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyWindow(const WindowId id)
{
    GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    GLFW::DisableCallbacks(id);
    delete (WindowId*)glfwGetWindowUserPointer(wnd);
    glfwDestroyWindow(wnd);

    CoreGraphics::SwapchainId swapchain = glfwWindowAllocator.Get<GLFW_Swapchain>(id.id);
    CoreGraphics::DestroySwapchain(swapchain);

    // close event
    GLFW::GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowClose, id));

    glfwWindowAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowResize(const WindowId id, SizeT newWidth, SizeT newHeight)
{
    GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    glfwSetWindowSize(wnd, newWidth, newHeight);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowSetTitle(const WindowId id, const Util::String & title)
{
    GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    glfwSetWindowTitle(wnd, title.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
void
WindowApplyFullscreen(const WindowId id, Adapter::Code monitor, bool b)
{
    GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    const CoreGraphics::DisplayMode& mode = glfwWindowAllocator.Get<GLFW_DisplayMode>(id.id);

    if (wnd && b)
    {
        GLFWmonitor* mon;
        if (monitor == Adapter::Primary)
        {
            mon = glfwGetPrimaryMonitor();
        }
        else
        {
            int count;
            GLFWmonitor** monitors = glfwGetMonitors(&count);
            n_assert(monitor < count);
            mon = monitors[monitor];
        }
        const GLFWvidmode* mode = glfwGetVideoMode(mon);
        glfwSetWindowMonitor(wnd, mon, 0, 0, mode->width, mode->height, 60);
    }
    else
    {
        glfwSetWindowMonitor(wnd, NULL, mode.GetXPos(), mode.GetYPos(), mode.GetWidth(), mode.GetHeight(), mode.GetRefreshRate());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WindowSetCursorVisible(const WindowId id, bool b)
{
    GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    glfwSetInputMode(wnd, GLFW_CURSOR, b ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    
}

//------------------------------------------------------------------------------
/**
*/
void
WindowSetCursorLocked(const WindowId id, bool b)
{
    GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
    glfwSetInputMode(wnd, GLFW_CURSOR, b ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

//------------------------------------------------------------------------------
/**
*/
void
WindowMakeCurrent(const WindowId id)
{
    CurrentWindow = id;
}

//------------------------------------------------------------------------------
/**
*/
void
WindowPresent(const WindowId id, const IndexT frameIndex)
{
    IndexT& frame = glfwWindowAllocator.Get<GLFW_SwapFrame>(id.id);
    if (frame != frameIndex)
    {
        CoreGraphics::SwapchainId swapchain = glfwWindowAllocator.Get<GLFW_Swapchain>(id.id);
        CoreGraphics::SwapchainPresent(swapchain);
        frame = frameIndex;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WindowPollEvents()
{
    glfwPollEvents();
}

//------------------------------------------------------------------------------
/**
*/
void
WindowNewFrame(const WindowId id)
{
    ResizeInfo& info = glfwWindowAllocator.Get<GLFW_ResizeInfo>(id.id);
    if (!info.done)
    {
        info.done = true;

        CoreGraphics::DisplayMode& mode = glfwWindowAllocator.Get<GLFW_DisplayMode>(id.id);
        mode.SetWidth(info.newWidth);
        mode.SetHeight(info.newHeight);
        mode.SetAspectRatio(info.newWidth / float(info.newHeight));

        // Destroy old swap chain and make a new one
        CoreGraphics::SwapchainId swapchain = glfwWindowAllocator.Get<GLFW_Swapchain>(id.id);
        CoreGraphics::DestroySwapchain(swapchain);

        GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
        CoreGraphics::SwapchainCreateInfo newSwapInfo;
        newSwapInfo.displayMode = mode;
        newSwapInfo.vsync = info.vsync;
        newSwapInfo.window = wnd;
        glfwWindowAllocator.Set<GLFW_Swapchain>(id.id, CoreGraphics::CreateSwapchain(newSwapInfo));

        // notify event listeners we resized
        GLFW::GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowResized, id));
    }
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::DisplayMode
WindowGetDisplayMode(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_DisplayMode>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::AntiAliasQuality::Code
WindowGetAAQuality(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_SetupInfo>(id.id).aa;
}

//------------------------------------------------------------------------------
/**
*/
const bool
WindowIsFullscreen(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_SetupInfo>(id.id).fullscreen;
}

//------------------------------------------------------------------------------
/**
*/
const bool
WindowIsDecorated(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_SetupInfo>(id.id).decorated;
}

//------------------------------------------------------------------------------
/**
*/
const bool
WindowIsResizable(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_SetupInfo>(id.id).resizable;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom&
WindowGetTitle(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_SetupInfo>(id.id).title;
}

//------------------------------------------------------------------------------
/**
*/
const Util::StringAtom&
WindowGetIcon(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_SetupInfo>(id.id).icon;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::SwapchainId
WindowGetSwapchain(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_Swapchain>(id.id);
}

} // namespace CoreGraphics

