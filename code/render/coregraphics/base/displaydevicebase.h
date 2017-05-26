#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::DisplayDeviceBase
  
    A DisplayDevice object represents the display where the RenderDevice
    presents the rendered frame. Use the display device object to 
    get information about available adapters and display modes, and
    to set the preferred display mode of a Nebula3 application.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/antialiasquality.h"
#include "coregraphics/adapter.h"
#include "coregraphics/displayeventhandler.h"
#include "coregraphics/displaymode.h"
#include "coregraphics/adapterinfo.h"
#include "util/blob.h"
#include "events/event.h"
#include "coregraphics/window.h"

//------------------------------------------------------------------------------
namespace Base
{
class DisplayDeviceBase : public Core::RefCounted
{
    __DeclareClass(DisplayDeviceBase);
    __DeclareSingleton(DisplayDeviceBase);
public:
    /// constructor
    DisplayDeviceBase();
    /// destructor
    virtual ~DisplayDeviceBase();

	/// open the display
	bool Open();
	/// close the display
	void Close();
	/// return true if display is currently open
	bool IsOpen() const;
	/// process window system messages, call this method once per frame
	void ProcessWindowMessages();
	/// reopens the display device which enables switching from display modes
	void Reopen();

	/// set if vertical sync should be used
	void SetVerticalSyncEnabled(bool b);
	/// get if vertical sync is enabled
	const bool IsVerticalSyncEnabled() const;

    /// return true if adapter exists
    bool AdapterExists(CoreGraphics::Adapter::Code adapter);
    /// get available display modes on given adapter
    Util::Array<CoreGraphics::DisplayMode> GetAvailableDisplayModes(CoreGraphics::Adapter::Code adapter, CoreGraphics::PixelFormat::Code pixelFormat);
    /// return true if a given display mode is supported
    bool SupportsDisplayMode(CoreGraphics::Adapter::Code adapter, const CoreGraphics::DisplayMode& requestedMode);
    /// get current adapter display mode (i.e. the desktop display mode)
    CoreGraphics::DisplayMode GetCurrentAdapterDisplayMode(CoreGraphics::Adapter::Code adapter);
    /// get general info about display adapter
    CoreGraphics::AdapterInfo GetAdapterInfo(CoreGraphics::Adapter::Code adapter);

    /// set display adapter (make sure adapter exists!)
    void SetAdapter(CoreGraphics::Adapter::Code a);
    /// get display adapter
	CoreGraphics::Adapter::Code GetAdapter() const;

	/// get if a window is running in full screen
	const bool IsFullscreen() const;

    /// attach a display event handler
    void AttachEventHandler(const Ptr<CoreGraphics::DisplayEventHandler>& h);
    /// remove a display event handler
    void RemoveEventHandler(const Ptr<CoreGraphics::DisplayEventHandler>& h);

	/// create a new window
	Ptr<CoreGraphics::Window> SetupWindow(const Util::String& title, const Util::String& icon, const CoreGraphics::DisplayMode& displayMode, const CoreGraphics::AntiAliasQuality::Code aa = CoreGraphics::AntiAliasQuality::None);
	/// create a window from one created by another window system
	Ptr<CoreGraphics::Window> EmbedWindow(const Util::Blob& windowData);
	/// get the 'main' window, if none exists, returns NULL
	Ptr<CoreGraphics::Window> GetMainWindow() const;
	/// get the current window
	const Ptr<CoreGraphics::Window>& GetCurrentWindow() const;
	/// get window using index, where 0 is the default window
	const Ptr<CoreGraphics::Window>& GetWindow(IndexT index) const;
	/// get all windows as an array
	const Util::Array<Ptr<CoreGraphics::Window>>& GetWindows() const;
	/// make ID the current one
	void MakeWindowCurrent(const IndexT index);
        
protected:
    /// notify event handlers about an event
    bool NotifyEventHandlers(const CoreGraphics::DisplayEvent& e);

    CoreGraphics::Adapter::Code adapter;

    bool verticalSync;
    bool isOpen;
	bool isFullscreen;

    Util::Array<Ptr<CoreGraphics::DisplayEventHandler> > eventHandlers;
	Ptr<CoreGraphics::Window> currentWindow;
	Util::Array<Ptr<CoreGraphics::Window>> windows;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
DisplayDeviceBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayDeviceBase::SetVerticalSyncEnabled(bool b)
{
	this->verticalSync = b;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
DisplayDeviceBase::IsVerticalSyncEnabled() const
{
	return this->verticalSync;
}

//------------------------------------------------------------------------------
/**
*/
inline void
DisplayDeviceBase::SetAdapter(CoreGraphics::Adapter::Code a)
{
    this->adapter = a;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::Adapter::Code
DisplayDeviceBase::GetAdapter() const
{
    return this->adapter;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
DisplayDeviceBase::IsFullscreen() const
{
	return this->isFullscreen;
}

//------------------------------------------------------------------------------
/**
*/
inline Ptr<CoreGraphics::Window>
DisplayDeviceBase::GetMainWindow() const
{
	if (this->windows.Size() >= 1) return this->windows[0];
	else						  return NULL;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Window>&
DisplayDeviceBase::GetCurrentWindow() const
{
	return this->currentWindow;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Window>&
DisplayDeviceBase::GetWindow(IndexT index) const
{
	n_assert(this->windows.Size() > index && index != InvalidIndex);
	return this->windows[index];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<CoreGraphics::Window>>&
DisplayDeviceBase::GetWindows() const
{
	return this->windows;
}

} // namespace Base
//------------------------------------------------------------------------------

