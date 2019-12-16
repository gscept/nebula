//------------------------------------------------------------------------------
//  displaydevicebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/base/displaydevicebase.h"
#include "frame/frameserver.h"

namespace Base
{
__ImplementClass(Base::DisplayDeviceBase, 'DSDB', Core::RefCounted);
__ImplementSingleton(Base::DisplayDeviceBase);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
DisplayDeviceBase::DisplayDeviceBase() :
    adapter(Adapter::Primary),
    verticalSync(true),
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
DisplayDeviceBase::~DisplayDeviceBase()
{
    n_assert(!this->IsOpen());
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Open the display.
*/
bool
DisplayDeviceBase::Open()
{
    n_assert(!this->IsOpen());
    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    Close the display.
*/
void
DisplayDeviceBase::Close()
{
    n_assert(this->IsOpen());
    this->isOpen = false;

	IndexT i;
	for (i = 0; i < this->windows.Size(); i++)
	{
		CoreGraphics::DestroyWindow(this->windows[i]);
	}
	this->windows.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
DisplayDeviceBase::Reopen()
{
	n_assert(this->IsOpen());

	// notify all event handlers
	DisplayEvent closeEvent(DisplayEvent::WindowReopen);
	this->NotifyEventHandlers(closeEvent);
}

//------------------------------------------------------------------------------
/**
    Attach an event handler to the display device.
*/
void
DisplayDeviceBase::AttachEventHandler(const Ptr<DisplayEventHandler>& handler)
{
    n_assert(handler.isvalid());
    n_assert(InvalidIndex == this->eventHandlers.FindIndex(handler));
    this->eventHandlers.Append(handler);
    handler->OnAttach();
}

//------------------------------------------------------------------------------
/**
    Remove an event handler from the display device.
*/
void
DisplayDeviceBase::RemoveEventHandler(const Ptr<DisplayEventHandler>& handler)
{
    n_assert(handler.isvalid());
    IndexT index = this->eventHandlers.FindIndex(handler);
    n_assert(InvalidIndex != index);
    this->eventHandlers.EraseIndex(index);
    handler->OnRemove();
}

//------------------------------------------------------------------------------
/**
    Notify all event handlers about an event.
*/
bool
DisplayDeviceBase::NotifyEventHandlers(const DisplayEvent& event)
{
    bool handled = false;
    IndexT i;
    for (i = 0; i < this->eventHandlers.Size(); i++)
    {
        handled |= this->eventHandlers[i]->PutEvent(event);
    }
    return handled;
}

//------------------------------------------------------------------------------
/**
    Process window system messages. Override this method in a subclass.
*/
void
DisplayDeviceBase::ProcessWindowMessages()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Returns the display modes on the given adapter in the given pixel format.
*/
Util::Array<DisplayMode>
DisplayDeviceBase::GetAvailableDisplayModes(Adapter::Code adapter, PixelFormat::Code pixelFormat)
{
    // override this method in a subclass to do something useful
    Util::Array<DisplayMode> emptyArray;
    return emptyArray;
}

//------------------------------------------------------------------------------
/**
    This method checks the available display modes on the given adapter
    against the requested display modes and returns true if the display mode
    exists.
*/
bool
DisplayDeviceBase::SupportsDisplayMode(Adapter::Code adapter, const DisplayMode& requestedMode)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    This method returns the current adapter display mode. It can be used
    to get the current desktop display mode.
*/
DisplayMode
DisplayDeviceBase::GetCurrentAdapterDisplayMode(Adapter::Code adapter)
{
    DisplayMode emptyMode;
    return emptyMode;
}

//------------------------------------------------------------------------------
/**
    Checks if the given adapter exists.
*/
bool
DisplayDeviceBase::AdapterExists(Adapter::Code adapter)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Returns information about the provided adapter.
*/
AdapterInfo
DisplayDeviceBase::GetAdapterInfo(Adapter::Code adapter)
{
    AdapterInfo emptyAdapterInfo;
    return emptyAdapterInfo;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::WindowId
DisplayDeviceBase::SetupWindow(const Util::String& title, const Util::String& icon, const CoreGraphics::DisplayMode& displayMode, const CoreGraphics::AntiAliasQuality::Code aa)
{
	WindowCreateInfo info;
	info.aa = aa;
	info.mode = displayMode;
	info.title = title;
	info.icon = icon;
	CoreGraphics::WindowId wnd = CoreGraphics::CreateWindow(info);

	// add to list, and set to current if this is the first
	if (this->windows.IsEmpty()) this->currentWindow = wnd;
	this->windows.Append(wnd);

	return wnd;
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::WindowId
DisplayDeviceBase::EmbedWindow(const Util::Blob& windowData)
{
	CoreGraphics::WindowId wnd = CoreGraphics::EmbedWindow(windowData);

	// add to list, and set to current if this is the first
	if (this->windows.IsEmpty()) this->currentWindow = wnd;
	this->windows.Append(wnd);
	return wnd;
}

//------------------------------------------------------------------------------
/**
*/
void
DisplayDeviceBase::MakeWindowCurrent(const CoreGraphics::WindowId id)
{
	CoreGraphics::WindowMakeCurrent(id);
	Frame::FrameServer::Instance()->SetWindowTexture(WindowGetTexture(id));
	this->currentWindow = id;
}

} // namespace DisplayDevice
