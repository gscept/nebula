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
};

/// create new window
const WindowId CreateWindow(const WindowCreateInfo& info);
/// embed window in another window
const WindowId EmbedWindow(const Util::Blob& windowData);
/// destroy window
void DestroyWindow(const WindowId id);

/// open window
void OpenWindow(const WindowId id);
/// close window
void CloseWindow(const WindowId id);
/// resize window
void ResizeWindow(const WindowId id, SizeT newWidth, SizeT newHeight);
/// make window 'current'
void MakeWindowCurrent(const WindowId id);
/// get display mode from window
const CoreGraphics::DisplayMode GetWindowDisplayMode(const WindowId id);

extern Ids::IdPool windowIdPool;

} // CoreGraphics
