#pragma once
//------------------------------------------------------------------------------
/**
    Window related stuff

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idpool.h"
#include "coregraphics/displaymode.h"
#include "coregraphics/antialiasquality.h"
#include "coregraphics/adapter.h"
#include "util/stringatom.h"
#include "util/blob.h"

#if defined(CreateWindow)
#undef CreateWindow
#endif

namespace CoreGraphics
{

ID_24_8_TYPE(WindowId);
struct TextureId;
struct SwapchainId;
struct CmdBufferId;
struct TextureId;


extern WindowId UpdatingWindow;
extern WindowId FocusWindow;
extern WindowId MainWindow;
struct WindowCreateInfo
{
    CoreGraphics::DisplayMode mode;
    Util::StringAtom title = nullptr;
    Util::StringAtom icon = nullptr;
    CoreGraphics::AntiAliasQuality::Code aa = CoreGraphics::AntiAliasQuality::None;
    void* userData = nullptr; // Setting user data implicitly states that another system owns and manages this window
    bool resizable : 1;
    bool decorated : 1;
    bool fullscreen : 1;
    bool vsync : 1;
};

/// create the main window
const WindowId CreateMainWindow(const WindowCreateInfo& info);
/// create new window
const WindowId CreateWindow(const WindowCreateInfo& info);
/// destroy window
void DestroyWindow(const WindowId id);

/// resize window
void WindowResize(const WindowId id, SizeT newWidth, SizeT newHeight);
/// Get window size
Math::int2 WindowGetSize(const WindowId id);
/// Set window position
void WindowReposition(const WindowId id, int x, int y);
/// Get window position
Math::int2 WindowGetPosition(const WindowId id);
/// set title for window
void WindowSetTitle(const WindowId id, const Util::String& title);
/// Set window icon
void WindowSetIcon(const WindowId id, const Util::String& icon);
/// Show window
void WindowShow(const WindowId id);
/// present window
void WindowPresent(const WindowId id, const IndexT frameIndex);
/// poll events for window
void WindowPollEvents();
/// Do new frame stuff for window
void WindowNewFrame(const WindowId id);
/// toggle fullscreen
void WindowApplyFullscreen(const WindowId id, Adapter::Code monitor, bool b);
/// set if the cursor should be visible
void WindowSetCursorVisible(const WindowId id, bool b);
/// set if the cursor should be locked to the window
void WindowSetCursorLocked(const WindowId id, bool b);
/// Focus window
void WindowTakeFocus(const WindowId id);

/// Get user data associated with window
void* WindowGetUserData(const WindowId id);

/// get display mode from window
const CoreGraphics::DisplayMode WindowGetDisplayMode(const WindowId id);
/// get display antialias quality
const CoreGraphics::AntiAliasQuality::Code WindowGetAAQuality(const WindowId id);
/// get if fullscreen
const bool WindowIsFullscreen(const WindowId id);
/// get if window is modal
const bool WindowIsDecorated(const WindowId id);
/// get if we are allowed to switch the display mode
const bool WindowIsResizable(const WindowId id);
/// get window title
const Util::StringAtom& WindowGetTitle(const WindowId id);
/// get window icon
const Util::StringAtom& WindowGetIcon(const WindowId id);
/// retrieve window content scaling (ratio between current DPI and platform default DPI)
Math::vec2 WindowGetContentScale(const WindowId id);
/// Get swapchain associated with this window
const CoreGraphics::SwapchainId WindowGetSwapchain(const WindowId id);

} // CoreGraphics
