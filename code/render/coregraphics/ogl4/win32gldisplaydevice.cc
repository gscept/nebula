//------------------------------------------------------------------------------
//  win32gldisplaydevice.cc
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "coregraphics/ogl4/ogl4displaydevice.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "coregraphics/ogl4/ogl4types.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::Win32GLDisplayDevice, 'O4WD', Win32::Win32DisplayDevice);
__ImplementSingleton(OpenGL4::Win32GLDisplayDevice);

using namespace Util;
using namespace CoreGraphics;
using namespace OpenGL4;

//------------------------------------------------------------------------------
/**
*/
Win32GLDisplayDevice::Win32GLDisplayDevice() 	
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
Win32GLDisplayDevice::~Win32GLDisplayDevice()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    This method checks if the given adapter actually exists.
*/
bool
Win32GLDisplayDevice::AdapterExists(Adapter::Code adapter)
{
	// Primary is always present in windows
	if(adapter == Adapter::Primary)
		return true;

	DWORD iNum	= 0 ;	
	DWORD iDevNum = 0 ;	
	DISPLAY_DEVICE ddi ;
	
//	TCHAR		szBuffer [100] ;
	ZeroMemory (&ddi, sizeof(ddi)) ;
	ddi.cb = sizeof(ddi) ;	

	while(EnumDisplayDevices (NULL, iDevNum++, &ddi, 0))
	{
		if( (ddi.StateFlags & DISPLAY_DEVICE_ACTIVE ) && !(ddi.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
			return true;		
		ZeroMemory (&ddi, sizeof(ddi)) ;
		ddi.cb = sizeof(ddi) ;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
    Enumerate the available display modes on the given adapter in the
    given pixel format. If the adapter doesn't exist on this machine,
    an empty array is returned.
*/
Util::Array<DisplayMode>
Win32GLDisplayDevice::GetAvailableDisplayModes(Adapter::Code adapter, PixelFormat::Code pixelFormat)
{

	n_assert(this->AdapterExists(adapter));
			
	HDC hdc = GetDC(0); 
	PIXELFORMATDESCRIPTOR  pfd; 
	
	int  iPixelFormat;
	iPixelFormat = 1; 

	Util::Array<PixelFormat::Code> formats;
	Util::Array<PixelFormat::Code> depths;

	// obtain detailed information about  
	// the device context's first pixel format  
	while(DescribePixelFormat(hdc, iPixelFormat++,  sizeof(PIXELFORMATDESCRIPTOR), &pfd))
	{			
		if((pfd.dwFlags & (PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER)) && (pfd.iPixelType == PFD_TYPE_RGBA))
		{
			if(pfd.cBlueBits == 8 && pfd.cRedBits == 8 && pfd.cGreenBits == 8 && pfd.cAlphaBits == 8 && pfd.cStencilBits == 8 && pfd.cDepthBits > 0)					
			{					
				switch(pfd.cDepthBits)
				{
					case 16:
						depths.Append(PixelFormat::D16S8);
						break;
					case 24:
						depths.Append(PixelFormat::D24S8);
						break;
					case 32:
						depths.Append(PixelFormat::D32S8);
						break;
					default:
						n_error("unknown depth in pixelsize\n");
						continue;
				}
				formats.Append(PixelFormat::A8B8G8R8);					
			}
		}			
	}
    
	DWORD		iModeNum = 0 ;
	DISPLAY_DEVICE	ddi ;
	DEVMODE		dmi ;
	DWORD		iDevNum	= 0 ;

	ZeroMemory (&ddi, sizeof(ddi));
	ddi.cb = sizeof(ddi);
	ZeroMemory (&dmi, sizeof(dmi));
	dmi.dmSize = sizeof(dmi);

	bool primary = adapter == Adapter::Primary ? true : false;

	while (EnumDisplayDevices (NULL, iDevNum++, &ddi, 0))
	{
		if( ddi.StateFlags & DISPLAY_DEVICE_ACTIVE )
		{
			if(primary && (ddi.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
				break;
			if(!primary && !(ddi.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
				break;
		
		}
		ZeroMemory (&ddi, sizeof(ddi)) ;
		ddi.cb = sizeof(ddi) ;
	}
	n_assert(ddi.StateFlags & DISPLAY_DEVICE_ACTIVE);


	Array<DisplayMode> modeArray;
	while (EnumDisplaySettings (ddi.DeviceName, iModeNum++, &dmi))
	{	
		DisplayMode m;
		m.SetWidth(dmi.dmPelsWidth);
		m.SetHeight(dmi.dmPelsHeight);
		m.SetRefreshRate(dmi.dmDisplayFrequency);
		if (modeArray.Size() == 0) modeArray.Append(m);
		else if (modeArray.FindIndex(m) == InvalidIndex) modeArray.Append(m);
		ZeroMemory (&dmi, sizeof(dmi)) ;		
	}	
	
    return modeArray;
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32GLDisplayDevice::SupportsDisplayMode(Adapter::Code adapter, const DisplayMode& requestedMode)
{
    Util::Array<DisplayMode> modes = this->GetAvailableDisplayModes(adapter, requestedMode.GetPixelFormat());
    return InvalidIndex != modes.FindIndex(requestedMode);
}

//------------------------------------------------------------------------------
/**
*/
DisplayMode
Win32GLDisplayDevice::GetCurrentAdapterDisplayMode(Adapter::Code adapter)
{
    n_assert(this->AdapterExists(adapter));
	//Array<DisplayMode> modeList = GetAvailableDisplayModes(adapter, PixelFormat::A8B8G8R8);
	//DisplayMode mode = modeList[0];
    return this->displayMode;
}

//------------------------------------------------------------------------------
/**
*/
AdapterInfo
Win32GLDisplayDevice::GetAdapterInfo(Adapter::Code adapter)
{
    n_assert(this->AdapterExists(adapter));

    AdapterInfo info;

	// there is no way to get any of this information using opengl, so instead we just ignore it
    info.SetDriverName((const char*) glGetString( GL_VENDOR ));		
	Util::String ver((const char*) glGetString( GL_VERSION ));
	float verfloat = ver.AsFloat();
	info.SetDescription("OpenGL Version: " + ver);	
	info.SetDeviceName(Util::String((const char*)glGetString(GL_RENDERER)));
    info.SetDriverVersionLowPart((int)((verfloat-(int)verfloat)*1000.0f));
    info.SetDriverVersionHighPart((int)verfloat); // and thus, it was most likely for an Nvidia card...
    info.SetVendorId(0);	
    info.SetDeviceId(0);
    info.SetSubSystemId(0);
    info.SetRevision(0);
	Guid dummy;
	dummy.Generate();
    info.SetGuid(dummy);
    return info;
}

//------------------------------------------------------------------------------
/**
    Compute an adjusted window rectangle, taking monitor info, 
    windowed/fullscreen etc... into account.
*/
DisplayMode
Win32GLDisplayDevice::ComputeAdjustedWindowRect()
{
    if (0 != this->parentWindow)
    {
#if __WIN32__
		return Win32DisplayDevice::ComputeAdjustedWindowRect();
#endif
    }
    else
    {
        // get monitor handle of selected adapter
        n_assert(this->AdapterExists(this->adapter));

		
        HMONITOR hMonitor = MonitorFromWindow(this->GetHwnd(), MONITOR_DEFAULTTOPRIMARY);
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
} // namespace CoreGraphics

