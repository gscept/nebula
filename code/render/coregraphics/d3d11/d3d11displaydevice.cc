DESC//------------------------------------------------------------------------------
//  d3d11displaydevice.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11displaydevice.h"
#include "coregraphics/d3d11/d3d11renderdevice.h"
#include "coregraphics/d3d11/d3d11types.h"

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11DisplayDevice, 'D1DD', Win32::Win32DisplayDevice);
__ImplementSingleton(Direct3D11::D3D11DisplayDevice);

using namespace Util;
using namespace CoreGraphics;
using namespace Direct3D11;

//------------------------------------------------------------------------------
/**
*/
D3D11DisplayDevice::D3D11DisplayDevice() :
	idxgiFactory(0),
	idxgiDevice(0),
	idxgiAdapter(0)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
D3D11DisplayDevice::~D3D11DisplayDevice()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    This method checks if the given adapter actually exists.
*/
bool
D3D11DisplayDevice::AdapterExists(Adapter::Code adapter)
{
	HRESULT hr;
	if (0 == idxgiFactory)
	{
		 hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&idxgiFactory);
	}
	
	GetAdapters();
    return (idxgiFactory != 0);
}

//------------------------------------------------------------------------------
/**
    Enumerate the available display modes on the given adapter in the
    given pixel format. If the adapter doesn't exist on this machine,
    an empty array is returned.
*/
Util::Array<DisplayMode>
D3D11DisplayDevice::GetAvailableDisplayModes(Adapter::Code adapter, PixelFormat::Code pixelFormat)
{
	n_assert(this->AdapterExists(adapter));
    HRESULT hr;
	IDXGIOutput* output = NULL;
	
	idxgiAdapter = adapterArray[adapter];
	hr = idxgiAdapter->EnumOutputs(0, &output);

	
	UINT numModes = 0;
	DXGI_MODE_DESC* displayModes = NULL;
	DXGI_FORMAT format = D3D11Types::AsD3D11PixelFormat(pixelFormat);
	Array<DisplayMode> modeArray;

	hr = output->GetDisplayModeList(format, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if(FAILED(hr))
	{
		/// probably doing rdp and microsoft is so awesome at it!
		/// lets just pretend it worked anyway
		DisplayMode failureAtBrain;
		failureAtBrain.SetPixelFormat(D3D11Types::AsNebulaPixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM));
		failureAtBrain.SetHeight(480);
		failureAtBrain.SetWidth(640);
		failureAtBrain.SetRefreshRate(60);
		modeArray.Append(failureAtBrain);
		
	}
	else
	{
		displayModes = new DXGI_MODE_DESC[numModes];

		hr = output->GetDisplayModeList(format, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModes);


		for (uint i = 0; i < numModes; i++)
		{
			DisplayMode mode;
			mode.SetRefreshRate(displayModes[i].RefreshRate.Numerator / displayModes[i].RefreshRate.Denominator);
			mode.SetWidth(displayModes[i].Width);
			mode.SetWidth(displayModes[i].Height);
			mode.SetPixelFormat(D3D11Types::AsNebulaPixelFormat(displayModes[i].Format));
			if (InvalidIndex == modeArray.FindIndex(mode))
			{
				modeArray.Append(mode);
			}
		}

	}
	
    return modeArray;
}

//------------------------------------------------------------------------------
/**
*/
bool
D3D11DisplayDevice::SupportsDisplayMode(Adapter::Code adapter, const DisplayMode& requestedMode)
{
    Util::Array<DisplayMode> modes = this->GetAvailableDisplayModes(adapter, requestedMode.GetPixelFormat());
    return InvalidIndex != modes.FindIndex(requestedMode);
}

//------------------------------------------------------------------------------
/**
*/
DisplayMode
D3D11DisplayDevice::GetCurrentAdapterDisplayMode(Adapter::Code adapter)
{
    n_assert(this->AdapterExists(adapter));
	Array<DisplayMode> modeList = GetAvailableDisplayModes(adapter, D3D11Types::AsNebulaPixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM));

	DisplayMode mode = modeList[0];
    return mode;
}

//------------------------------------------------------------------------------
/**
*/
AdapterInfo
D3D11DisplayDevice::GetAdapterInfo(Adapter::Code adapter)
{
    n_assert(this->AdapterExists(adapter));
	IDXGIAdapter* currentAdapter = adapterArray[adapter];
	DXGI_ADAPTER_DESC adapterDesc;
	
	HRESULT hr = currentAdapter->GetDesc(&adapterDesc);
    n_assert(SUCCEEDED(hr));
	
    AdapterInfo info;
	//Some of the information is not accessible using DirectX 11. AdapterInfo is not platform transparent, so the unnecessary data is left with crap.
    info.SetDriverName("<insert DirectX 11 driver name here>");
	
	info.SetDescription(Util::String((const char*)adapterDesc.Description));
    info.SetDeviceName("Grats, either you have a GeForce 500 series, or HD5000-series graphics card, or better!");
    info.SetDriverVersionLowPart(13); // this version of the driver was the best ever made
    info.SetDriverVersionHighPart(37); // and thus, it was most likely for an Nvidia card...
    info.SetVendorId(adapterDesc.VendorId);
    info.SetDeviceId(adapterDesc.DeviceId);
    info.SetSubSystemId(adapterDesc.SubSysId);
    info.SetRevision(adapterDesc.Revision);
    //info.SetGuid(Guid((const unsigned char*) &(adapterDesc.AdapterLuid), sizeof(adapterDesc.AdapterLuid)));
    return info;
}

//------------------------------------------------------------------------------
/**
    Compute an adjusted window rectangle, taking monitor info, 
    windowed/fullscreen etc... into account.
*/
DisplayMode
D3D11DisplayDevice::ComputeAdjustedWindowRect()
{
    if (0 != this->windowData.GetPtr())
    {
        // if we're a child window, let parent class handle it
        return Win32DisplayDevice::ComputeAdjustedWindowRect();
    }
    else
    {
        // get monitor handle of selected adapter
	
        n_assert(this->AdapterExists(this->adapter));

		IDXGIOutput* output = NULL;

		IDXGIAdapter* currentAdapter = adapterArray[this->adapter];
		currentAdapter->EnumOutputs(0, &output);

		DXGI_OUTPUT_DESC desc;

		output->GetDesc(&desc);

        HMONITOR hMonitor = desc.Monitor;
        MONITORINFO monitorInfo = { sizeof(monitorInfo), 0 };
        GetMonitorInfo(hMonitor, &monitorInfo);
        int monitorOriginX = monitorInfo.rcMonitor.left;
        int monitorOriginY = monitorInfo.rcMonitor.top;
        
        if (this->fullscreen)
        {
            // running in fullscreen mode
            if (!this->IsDisplayModeSwitchEnabled())
            {
                // running in "fake" fullscreen mode center the window on the desktop
                int monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
                int monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
                int x = monitorOriginX + ((monitorWidth - this->displayMode.GetWidth()) / 2);
                int y = monitorOriginY + ((monitorHeight - this->displayMode.GetHeight()) / 2);
                return DisplayMode(x, y, this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->displayMode.GetPixelFormat());
            }
            else
            {
                // running in normal fullscreen mode
                return DisplayMode(monitorOriginX, monitorOriginY, this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->displayMode.GetPixelFormat());
            }
        }
        else
        {
            // need to make sure that the window client area is the requested width
            const DisplayMode& mode = this->displayMode;
            int adjXPos = monitorOriginX + mode.GetXPos();
            int adjYPos = monitorOriginY + mode.GetYPos();
            RECT adjRect;
            adjRect.left   = adjXPos;
            adjRect.right  = adjXPos + mode.GetWidth();
            adjRect.top    = adjYPos;
            adjRect.bottom = adjYPos + mode.GetHeight();
            AdjustWindowRect(&adjRect, this->windowedStyle, 0);
            int adjWidth = adjRect.right - adjRect.left;
            int adjHeight = adjRect.bottom - adjRect.top;
            return DisplayMode(adjXPos, adjYPos, adjWidth, adjHeight, this->displayMode.GetPixelFormat());
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11DisplayDevice::GetAdapters()
{
	if (adapterArray.Size() == 0)
	{
		IDXGIAdapter* currentAdapter;
		
		for (UINT i = 0; idxgiFactory->EnumAdapters(i, &currentAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			adapterArray.Append(currentAdapter);
		}
	}
}

} // namespace CoreGraphics
