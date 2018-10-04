#pragma once
//------------------------------------------------------------------------------
/**
	Base class for windows, implements a basic API for opening, closing and resizing windows.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/displaymode.h"
#include "coregraphics/antialiasquality.h"
#include "coregraphics/rendertexture.h"
namespace Base
{
class WindowBase : public Core::RefCounted
{
	__DeclareClass(WindowBase);
public:
	/// constructor
	WindowBase();
	/// destructor
	virtual ~WindowBase();

	/// set window title
	void SetTitle(const Util::String& title);
	/// get window title
	const Util::String& GetTitle() const;
	/// set name of icon
	void SetIconName(const Util::String& icon);
	/// get name of icon
	const Util::String& GetIconName() const;
	/// set window data (used when some other system created the window)
	void SetWindowData(const Util::Blob& data);
	/// get window unique id
	const IndexT& GetWindowId() const;

	/// set windowed/fullscreen mode, monitor decides which monitor to use
	void SetFullscreen(bool b, int monitor = -1);
	/// get windowed/fullscreen mode
	bool IsFullscreen() const;
	/// enable display mode switch when running fullscreen (default is true);
	void SetDisplayModeSwitchEnabled(bool b);
	/// is display mode switch enabled for fullscreen?
	bool IsDisplayModeSwitchEnabled() const;
	/// enable triple buffer for fullscreen (default is double buffering)
	void SetTripleBufferingEnabled(bool b);
	/// is triple buffer enabled for fullscreen?
	bool IsTripleBufferingEnabled() const;
	/// set always-on-top behaviour
	void SetAlwaysOnTop(bool b);
	/// get always-on-top behaviour
	bool IsAlwaysOnTop() const;
	/// set optional embedding property
	void SetEmbedded(bool b);
	/// return optional embedding property
	bool IsEmbedded();
	/// set allow resize flag
	void SetResizable(bool enable);
	/// get allow resize flag
	bool IsResizable() const;
	/// set decorated flag
	void SetDecorated(bool enable);
	/// get decorated flag
	bool IsDecorated() const;
	/// set display mode (make sure the display mode is supported!)
	void SetDisplayMode(const CoreGraphics::DisplayMode& m);
	/// get display mode
	const CoreGraphics::DisplayMode& GetDisplayMode() const;
	/// set antialias quality
	void SetAntiAliasQuality(CoreGraphics::AntiAliasQuality::Code aa);
	/// get antialias quality
	CoreGraphics::AntiAliasQuality::Code GetAntiAliasQuality() const;
	/// set cursor visible
	void SetCursorVisible(bool visible);
	/// set cursor locked to window
	void SetCursorLocked(bool locked);

	/// enables callbacks
	void EnableCallbacks();
	/// disables callbacks
	void DisableCallbacks();

	/// open window
	void Open();
	/// close window
	void Close();
	/// reopen window (using new width and height)
	void Reopen();

	/// makes this window current, making future render commands go to this window
	void MakeCurrent();

	/// get render target used for this window backbuffer
	const Ptr<CoreGraphics::RenderTexture>& GetRenderTexture() const;
protected:
	CoreGraphics::DisplayMode displayMode;
	CoreGraphics::AntiAliasQuality::Code antiAliasQuality;
	Ptr<CoreGraphics::RenderTexture> defaultRenderTexture;

	/// resizes default render target
	void Resize(SizeT width, SizeT height);


	Util::String windowTitle;
	Util::String iconName;
	Util::Blob windowData;

	IndexT windowId;
	static IndexT uniqueWindowCounter;
	IndexT swapFrame;

	bool fullscreen;
	int monitor;
	bool modeSwitchEnabled;
	bool tripleBufferingEnabled;
	bool alwaysOnTop;
	bool embedded;
	bool resizable;
	bool decorated;
	bool cursorVisible;
	bool cursorLocked;
};

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetTitle(const Util::String& title)
{
	this->windowTitle = title;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
WindowBase::GetTitle() const
{
	return this->windowTitle;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetIconName(const Util::String& icon)
{
	this->iconName = icon;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
WindowBase::GetIconName() const
{
	return this->iconName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetWindowData(const Util::Blob& data)
{
	this->windowData = data;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT&
WindowBase::GetWindowId() const
{
	return this->windowId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetFullscreen(bool b, int monitor)
{
	this->fullscreen = b;
	this->monitor = monitor;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
WindowBase::IsFullscreen() const
{
	return this->fullscreen;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetDisplayModeSwitchEnabled(bool b)
{
	this->modeSwitchEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
WindowBase::IsDisplayModeSwitchEnabled() const
{
	return this->modeSwitchEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetTripleBufferingEnabled(bool b)
{
	this->tripleBufferingEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
WindowBase::IsTripleBufferingEnabled() const
{
	return this->tripleBufferingEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetAlwaysOnTop(bool b)
{
	this->alwaysOnTop = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
WindowBase::IsAlwaysOnTop() const
{
	return this->alwaysOnTop;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetEmbedded(bool b)
{
	this->embedded = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
WindowBase::IsEmbedded()
{
	return this->embedded;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
WindowBase::SetResizable(bool enable)
{
	this->resizable = enable;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
WindowBase::IsResizable() const
{
	return this->resizable;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
WindowBase::SetDecorated(bool enable)
{
	this->decorated = enable;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
WindowBase::IsDecorated() const
{
	return this->decorated;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetDisplayMode(const CoreGraphics::DisplayMode& m)
{
	this->displayMode = m;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::DisplayMode&
WindowBase::GetDisplayMode() const
{
	return this->displayMode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetAntiAliasQuality(CoreGraphics::AntiAliasQuality::Code aa)
{
	this->antiAliasQuality = aa;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::AntiAliasQuality::Code
WindowBase::GetAntiAliasQuality() const
{
	return this->antiAliasQuality;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetCursorVisible(bool visible)
{
	this->cursorVisible = visible;
}

//------------------------------------------------------------------------------
/**
*/
inline void
WindowBase::SetCursorLocked(bool locked)
{
	this->cursorLocked = locked;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::RenderTexture>&
WindowBase::GetRenderTexture() const
{
	return this->defaultRenderTexture;
}

} // namespace Base