#pragma once
//------------------------------------------------------------------------------
/**
	Window related stuff

	(C) 2017 Individual contributors, see AUTHORS file
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

struct WindowCreateInfo
{
	CoreGraphics::DisplayMode mode;
	Util::StringAtom title;
	Util::StringAtom icon;
	CoreGraphics::AntiAliasQuality::Code aa;
	bool resizable : 1;
	bool decorated : 1;
	bool fullscreen : 1;
};

/// create new window
const WindowId CreateWindow(const WindowCreateInfo& info);
/// embed window in another window
const WindowId EmbedWindow(const Util::Blob& windowData);
/// destroy window
void DestroyWindow(const WindowId id);

/// resize window
void WindowResize(const WindowId id, SizeT newWidth, SizeT newHeight);
/// set title for window
void WindowSetTitle(const WindowId id, const Util::String& title);
/// make window 'current'
void WindowMakeCurrent(const WindowId id);
/// present window
void WindowPresent(const WindowId id, const IndexT frameIndex);
/// toggle fullscreen
void WindowApplyFullscreen(const WindowId id, Adapter::Code monitor, bool b);
/// set if the cursor should be visible
void WindowSetCursorVisible(const WindowId id, bool b);
/// set if the cursor should be locked to the window
void WindowSetCursorLocked(const WindowId id, bool b);
/// get display mode from window
const CoreGraphics::DisplayMode WindowGetDisplayMode(const WindowId id);

extern Ids::IdPool windowIdPool;

} // CoreGraphics
