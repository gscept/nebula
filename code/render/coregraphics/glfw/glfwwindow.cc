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

#if __VULKAN__
#include "coregraphics/vk/vkgraphicsdevice.h"
#include "coregraphics/vk/vktypes.h"
#endif

namespace CoreGraphics
{

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
callback for key events
*/
void
staticKeyFunc(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    KeyFunc(*id, key, scancode, action, mods);
}

//------------------------------------------------------------------------------
/**
callbacks for character input
*/
void
staticCharFunc(GLFWwindow* window, unsigned int key)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    CharFunc(*id, key);
}

//------------------------------------------------------------------------------
/**
*/
void
staticMouseButtonFunc(GLFWwindow* window, int button, int action, int mods)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    MouseButtonFunc(*id, button, action, mods);
}

//------------------------------------------------------------------------------
/**
callbacks for mouse position
*/
void
staticMouseFunc(GLFWwindow* window, double xpos, double ypos)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    MouseFunc(*id, xpos, ypos);
}

//------------------------------------------------------------------------------
/**
callbacks for scroll event
*/
void
staticScrollFunc(GLFWwindow* window, double xs, double ys)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    ScrollFunc(*id, xs, ys);
}

//------------------------------------------------------------------------------
/**
callback for close requested
*/
void
staticCloseFunc(GLFWwindow* window)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    CloseFunc(*id);
}

//------------------------------------------------------------------------------
/**
callback for focus
*/
void
staticFocusFunc(GLFWwindow* window, int focus)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    FocusFunc(*id, focus);
}

//------------------------------------------------------------------------------
/**
*/
void
staticSizeFunc(GLFWwindow* window, int width, int height)
{
    CoreGraphics::WindowId* id = (CoreGraphics::WindowId*)glfwGetWindowUserPointer(window);
    ResizeFunc(*id, width, height);
}

//------------------------------------------------------------------------------
/**
*/
void
staticDropFunc(GLFWwindow* window, int files, const char** args)
{
    // empty for now
}

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
    glfwSetKeyCallback(window, staticKeyFunc);
    glfwSetMouseButtonCallback(window, staticMouseButtonFunc);
    glfwSetCursorPosCallback(window, staticMouseFunc);
    glfwSetWindowCloseCallback(window, staticCloseFunc);
    glfwSetWindowFocusCallback(window, staticFocusFunc);
    glfwSetWindowSizeCallback(window, staticSizeFunc);
    glfwSetScrollCallback(window, staticScrollFunc);
    glfwSetCharCallback(window, staticCharFunc);
    glfwSetDropCallback(window, staticDropFunc);
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
    glfwWindowAllocator.Set<GLFW_ResizeInfo>(windowId, { 0, 0, true, info.vsync });

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

    glfwSwapInterval(info.vsync ? 1 : 0);

#if __VULKAN__
    Vulkan::VkSwapchainInfo& swapInfo = glfwWindowAllocator.Get<GLFW_SwapChain>(id.id);
    VkResult res = glfwCreateWindowSurface(Vulkan::GetInstance(), wnd, nullptr, &swapInfo.surface);
    n_assert(res == VK_SUCCESS);

    // guh, we have to make the window current twice...
    DisplayDevice::Instance()->MakeWindowCurrent(id);

    // setup swapchain
    Vulkan::SetupVulkanSwapchain(id, info.mode, info.vsync, info.title);
#endif

    glfwWindowAllocator.Get<GLFW_Window>(windowId) = wnd;

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

#if __VULKAN__
    // discard swapchain
    Vulkan::DiscardVulkanSwapchain(id);
    // wait for queues to empty
    VkWindowSwapInfo& wndInfo = glfwWindowAllocator.Get<GLFW_WindowSwapInfo>(id.id);
    CoreGraphics::WaitAndClearPendingCommands();
    vkDeviceWaitIdle(wndInfo.dev);
#endif

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
    // FIXME
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
#if __VULKAN__
        Vulkan::Present(id);
#elif
        GLFWwindow* wnd = glfwWindowAllocator.Get<GLFW_Window>(id.id);
        glfwSwapBuffers(wnd);
#endif
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

        // resize default render target
        // WindowResize(id, width, height);
#if __VULKAN__
        // recreate swapchain
        Vulkan::RecreateVulkanSwapchain(id, mode, info.vsync, "RESIZED"_atm);
#endif

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
const CoreGraphics::TextureId&
WindowGetTexture(const WindowId id)
{
    return glfwWindowAllocator.Get<GLFW_Texture>(id.id);
}

} // namespace CoreGraphics

#ifdef __VULKAN__
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
const VkSurfaceKHR&
GetSurface(const CoreGraphics::WindowId& id)
{
    const VkSwapchainInfo& swapInfo = glfwWindowAllocator.Get<GLFW_SwapChain>(id.id);
    return swapInfo.surface;
}

//------------------------------------------------------------------------------
/**
*/
void
SetupVulkanSwapchain(const CoreGraphics::WindowId& id, const CoreGraphics::DisplayMode& mode, bool vsync, const Util::StringAtom& title)
{
    VkWindowSwapInfo& windowInfo = glfwWindowAllocator.Get<GLFW_WindowSwapInfo>(id.id);
    VkSwapchainInfo& swapInfo = glfwWindowAllocator.Get<GLFW_SwapChain>(id.id);
    VkBackbufferInfo& backbufferInfo = glfwWindowAllocator.Get<GLFW_Backbuffer>(id.id);

    VkPhysicalDevice physicalDev = Vulkan::GetCurrentPhysicalDevice();
    VkDevice dev = Vulkan::GetCurrentDevice();

    // find available surface formats
    uint32_t numFormats;
    VkResult res;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, swapInfo.surface, &numFormats, nullptr);
    n_assert(res == VK_SUCCESS);

    Util::FixedArray<VkSurfaceFormatKHR> formats(numFormats);
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDev, swapInfo.surface, &numFormats, formats.Begin());
    n_assert(res == VK_SUCCESS);
    swapInfo.format = formats[0].format;
    swapInfo.colorSpace = formats[0].colorSpace;
    VkComponentMapping mapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    uint32_t i;
    for (i = 0; i < numFormats; i++)
    {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            swapInfo.format = formats[i].format;
            swapInfo.colorSpace = formats[i].colorSpace;
            mapping.r = VK_COMPONENT_SWIZZLE_R;
            mapping.b = VK_COMPONENT_SWIZZLE_B;
            break;
        }
    }

    // get surface capabilities
    VkSurfaceCapabilitiesKHR surfCaps;

    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDev, swapInfo.surface, &surfCaps);
    n_assert(res == VK_SUCCESS);

    uint32_t numPresentModes;
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, swapInfo.surface, &numPresentModes, nullptr);
    n_assert(res == VK_SUCCESS);

    // get present modes
    Util::FixedArray<VkPresentModeKHR> presentModes(numPresentModes);
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDev, swapInfo.surface, &numPresentModes, presentModes.Begin());
    n_assert(res == VK_SUCCESS);

    VkExtent2D swapchainExtent;
    if (surfCaps.currentExtent.width == -1)
    {
        swapchainExtent.width = mode.GetWidth();
        swapchainExtent.height = mode.GetHeight();
    }
    else
    {
        swapchainExtent = surfCaps.currentExtent;
    }

    // figure out the best present mode, mailo
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (i = 0; i < numPresentModes; i++)
    {
        switch (presentModes[i])
        {
        case VK_PRESENT_MODE_MAILBOX_KHR:
            swapchainPresentMode = presentModes[i];
            numPresentModes = 0;
            break;
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            if (!vsync)
            {
                swapchainPresentMode = presentModes[i];
                numPresentModes = 0;
            }               
            break;
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            if (!vsync)
            {
                swapchainPresentMode = presentModes[i];
                numPresentModes = 0;
            }
            break;
        case VK_PRESENT_MODE_FIFO_KHR:
            continue;
        default:
            n_error("unhandled enum"); break;
        }
    }

    // use at least as many swap images as we have buffered frames, if we don't have enough, the swap chain creation will fail
    uint32_t numSwapchainImages = Math::max(surfCaps.minImageCount, Math::min((uint32_t)CoreGraphics::GetNumBufferedFrames(), surfCaps.maxImageCount));

    // create a transform
    VkSurfaceTransformFlagBitsKHR transform;
    if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    else                                                                      transform = surfCaps.currentTransform;

    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    n_assert((usageFlags & surfCaps.supportedUsageFlags) != 0);
    VkSwapchainCreateInfoKHR swapchainInfo =
    {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        swapInfo.surface,
        numSwapchainImages,
        swapInfo.format,
        swapInfo.colorSpace,
        swapchainExtent,
        1,
        usageFlags,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        transform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        swapchainPresentMode,
        VK_TRUE,
        VK_NULL_HANDLE
    };

    // get present queue
    for (IndexT i = 0; i < NumQueueTypes; i++)
    {
        VkBool32 canPresent;
        res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDev, i, swapInfo.surface, &canPresent);
        n_assert(res == VK_SUCCESS);
        if (canPresent)
        {
            windowInfo.presentQueue = Vulkan::GetQueue((CoreGraphics::QueueType)i, 0);
            break;
        }
    }

    // create swapchain
    res = vkCreateSwapchainKHR(dev, &swapchainInfo, nullptr, &windowInfo.swapchain);
    n_assert(res == VK_SUCCESS);

    res = vkGetSwapchainImagesKHR(dev, windowInfo.swapchain, &backbufferInfo.numBackbuffers, nullptr);
    n_assert(res == VK_SUCCESS);

    // get number of buffered frames from the graphics device, and limit the amount of backbuffers
    backbufferInfo.backbuffers.Resize(backbufferInfo.numBackbuffers);

    res = vkGetSwapchainImagesKHR(dev, windowInfo.swapchain, &backbufferInfo.numBackbuffers, backbufferInfo.backbuffers.Begin());
    n_assert(res == VK_SUCCESS);

    backbufferInfo.backbufferViews.Resize(backbufferInfo.numBackbuffers);
    for (i = 0; i < backbufferInfo.numBackbuffers; i++)
    {
        // setup view
        VkImageViewCreateInfo backbufferViewInfo =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            backbufferInfo.backbuffers[i],
            VK_IMAGE_VIEW_TYPE_2D,
            swapInfo.format,
            mapping,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };
        res = vkCreateImageView(dev, &backbufferViewInfo, nullptr, &backbufferInfo.backbufferViews[i]);
        n_assert(res == VK_SUCCESS);
    }
    windowInfo.currentBackbuffer = 0;

    TextureCreateInfo rtinfo;
    rtinfo.name = Util::String::Sprintf("__WINDOW__%s", title.Value());
    rtinfo.usage = TextureUsage::TransferTextureDestination;
    rtinfo.tag = "system"_atm;
    rtinfo.type = Texture2D;
    rtinfo.format = VkTypes::AsNebulaPixelFormat(swapInfo.format);
    rtinfo.width = (float)swapchainExtent.width;
    rtinfo.height = (float)swapchainExtent.height;
    rtinfo.windowTexture = true;
    rtinfo.defaultLayout = CoreGraphics::ImageLayout::TransferSource;
    glfwWindowAllocator.Get<GLFW_Texture>(id.id) = CreateTexture(rtinfo);

    // add to graphics device for swapbuffers
    CoreGraphics::AddBackBufferTexture(glfwWindowAllocator.Get<GLFW_Texture>(id.id));

    windowInfo.dev = dev;
}

//------------------------------------------------------------------------------
/**
*/
void
DiscardVulkanSwapchain(const CoreGraphics::WindowId& id)
{
    VkWindowSwapInfo& wndInfo = glfwWindowAllocator.Get<GLFW_WindowSwapInfo>(id.id);
    VkBackbufferInfo& backbufferInfo = glfwWindowAllocator.Get<GLFW_Backbuffer>(id.id);

    uint32_t i;
    for (i = 0; i < backbufferInfo.numBackbuffers; i++)
    {
        vkDestroyImageView(wndInfo.dev, backbufferInfo.backbufferViews[i], nullptr);
    }

    // destroy swapchain last
    vkDestroySwapchainKHR(wndInfo.dev, wndInfo.swapchain, nullptr);
    
    CoreGraphics::RemoveBackBufferTexture(glfwWindowAllocator.Get<GLFW_Texture>(id.id));

    // destroy __WINDOW__ render texture
    DestroyTexture(glfwWindowAllocator.Get<GLFW_Texture>(id.id));
}

//------------------------------------------------------------------------------
/**
*/
void
RecreateVulkanSwapchain(const CoreGraphics::WindowId& id, const CoreGraphics::DisplayMode& mode, bool vsync, const Util::StringAtom& title)
{
    // Wait until GPU is idle
    CoreGraphics::WaitAndClearPendingCommands();

    DiscardVulkanSwapchain(id);

    // TODO: We could pass the old swapchain when creating the new one, allowing any pending drawing to be finished before changing
    SetupVulkanSwapchain(id, mode, vsync, title);
}

//------------------------------------------------------------------------------
/**
*/
void
Present(const CoreGraphics::WindowId& id)
{
    const VkWindowSwapInfo& wndInfo = glfwWindowAllocator.Get<GLFW_WindowSwapInfo>(id.id);

    VkSemaphore semaphores[] =
    {
        Vulkan::GetRenderingSemaphore() // this will be the final semaphore of the graphics command buffer that finishes the frame
    };

#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::QueueBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLACK, "Presentation");
#endif

    const VkPresentInfoKHR info =
    {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1,
        semaphores,
        1,
        &wndInfo.swapchain,
        &wndInfo.currentBackbuffer,
        nullptr
    };

    // present
    VkResult res = vkQueuePresentKHR(wndInfo.presentQueue, &info);
    switch (res)
    {
        case VK_SUCCESS:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            break;
        default:
            n_error("Present failed");
    }


#if NEBULA_GRAPHICS_DEBUG
    CoreGraphics::QueueEndMarker(GraphicsQueueType);
#endif
}

} // namespace Vulkan

#endif // __VULKAN__
