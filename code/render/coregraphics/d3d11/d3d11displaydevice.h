#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11DisplayDevice
    
    Direct3D11 implementation of DisplayDevice class. Manages the application
    window.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/win32/win32displaydevice.h"
#include "util/array.h"

namespace Direct3D11
{
class D3D11DisplayDevice : public Win32::Win32DisplayDevice
{
    __DeclareClass(D3D11DisplayDevice);
    __DeclareSingleton(D3D11DisplayDevice);
public:
    /// constructor
    D3D11DisplayDevice();
    /// destructor
    virtual ~D3D11DisplayDevice();
    /// check if the adapter actually exists
    bool AdapterExists(CoreGraphics::Adapter::Code adapter);
	/// get all available adapters
	void GetAdapters();
    /// get available display modes on given adapter
    Util::Array<CoreGraphics::DisplayMode> GetAvailableDisplayModes(CoreGraphics::Adapter::Code adapter, CoreGraphics::PixelFormat::Code pixelFormat);
    /// return true if a given display mode is supported
    bool SupportsDisplayMode(CoreGraphics::Adapter::Code adapter, const CoreGraphics::DisplayMode& requestedMode);
    /// get current adapter display mode (i.e. the desktop display mode)
    CoreGraphics::DisplayMode GetCurrentAdapterDisplayMode(CoreGraphics::Adapter::Code adapter);
    /// get general info about display adapter
    CoreGraphics::AdapterInfo GetAdapterInfo(CoreGraphics::Adapter::Code adapter);

private:
    /// adjust window size taking client area into account
    virtual CoreGraphics::DisplayMode ComputeAdjustedWindowRect();

	/// the DirectX Graphics Infrastructure factory
	IDXGIFactory* idxgiFactory;

	/// the DirectX Graphics Infrastructure device
	IDXGIDevice* idxgiDevice;

	/// the DirectX Graphics Infrastructure adapter
	IDXGIAdapter* idxgiAdapter;

	/// An array that holds all available adapters
	Util::Array<IDXGIAdapter*> adapterArray;
	
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
    