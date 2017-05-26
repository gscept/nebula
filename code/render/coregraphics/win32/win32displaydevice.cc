//------------------------------------------------------------------------------
//  win32displaydevice.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/win32/win32displaydevice.h"
#include "coregraphics/renderdevice.h"
#include "rendermodules/rt/rtpluginregistry.h"

namespace Win32
{
__ImplementClass(Win32::Win32DisplayDevice, 'W32D', Base::DisplayDeviceBase);
__ImplementSingleton(Win32::Win32DisplayDevice);

using namespace CoreGraphics;
using namespace Math;

struct ParentWindow
{
	int width, height;
	HWND window;
};

//------------------------------------------------------------------------------
/**
*/
Win32DisplayDevice::Win32DisplayDevice() :
    hInst(0),
    hWnd(0),
	parentHwnd(0),
    hAccel(0),
    windowedStyle(WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE),
    childWindowStyle(WS_VISIBLE | WS_POPUP),
    fullscreenStyle(WS_POPUP | WS_VISIBLE)
{
    __ConstructSingleton;
    this->hInst = GetModuleHandle(0);
}

//------------------------------------------------------------------------------
/**
*/
Win32DisplayDevice::~Win32DisplayDevice()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32DisplayDevice::Open()
{
    n_assert(!this->IsOpen());
    if (DisplayDeviceBase::Open())
    {
		bool success;
		if (this->embedded)
		{
			success = this->EmbedWindow();
		}
		else
		{
			success = this->OpenWindow();
		}		
        return success;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::Close()
{
    n_assert(this->IsOpen());
    this->CloseWindow();
    DisplayDeviceBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
Win32DisplayDevice::Reopen()
{
	n_assert(this->IsOpen());

	// embedded windows require their own reopen by external system	
	if (this->parentHwnd)
	{
		SetParent(this->hWnd, parentHwnd);
		SetWindowLong(this->hWnd, GWL_STYLE, this->childWindowStyle);
	}
	else
	{
		if (this->IsFullscreen())
		{
			SetWindowLong(this->hWnd, GWL_STYLE, this->fullscreenStyle);
		}
		else
		{
			SetWindowLong(this->hWnd, GWL_STYLE, this->windowedStyle);
		}
		SetParent(this->hWnd, parentHwnd);
		SetWindowLong(this->hWnd, GWL_WNDPROC, (LONG)WinProc);
	}

	DisplayMode adjMode = this->ComputeAdjustedWindowRect();
	MoveWindow(this->hWnd, adjMode.GetXPos(), adjMode.GetYPos(), adjMode.GetWidth(), adjMode.GetHeight(), TRUE);
	DisplayDeviceBase::Reopen();
}

//------------------------------------------------------------------------------
/**
    Polls for and processes window messages. Call this message once per
    frame in your render loop. If the user clicks the window close 
    button, or hits Alt-F4, a CloseRequested input event will be sent.
*/
void
Win32DisplayDevice::ProcessWindowMessages()
{
    n_assert(this->IsOpen());
    
    // process messages if we have a window
    if (0 != this->hWnd)
    {
        n_assert(0 != this->hAccel);
        MSG msg;
        while (PeekMessage(&msg, this->hWnd, 0, 0, PM_REMOVE))
        {
            int msgHandled = TranslateAccelerator(this->hWnd, this->hAccel, &msg);
            if (0 == msgHandled)
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

}

//------------------------------------------------------------------------------
/**
    Open the application window.
*/
bool
Win32DisplayDevice::OpenWindow()
{
    n_assert(0 != this->hInst);
    n_assert(0 == this->hWnd);
    n_assert(0 == this->hAccel);

    // initialize accelerator keys
    ACCEL acc[1];
    acc[0].fVirt = FALT|FNOINVERT|FVIRTKEY;
    acc[0].key   = VK_RETURN;
    acc[0].cmd   = AccelToggleFullscreen;
    this->hAccel = CreateAcceleratorTable(acc, 1);

    // initialize application icon
    HICON icon = 0;
    if (this->iconName.IsValid())
    {
        icon = LoadIcon(this->hInst, this->iconName.AsCharPtr());
    }
    // fallthrough if no custom icon defined, or loading custom icon failed
    if (0 == icon)
    {
        icon = LoadIcon(NULL, IDI_APPLICATION);
    }

    // register window class
    WNDCLASSEX wndClass;
    Memory::Clear(&wndClass, sizeof(wndClass));
    wndClass.cbSize        = sizeof(wndClass);
    wndClass.style         = CS_DBLCLKS | CS_OWNDC;
    wndClass.lpfnWndProc   = WinProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(void*);   // used to hold 'this' pointer
    wndClass.hInstance     = this->hInst;
    wndClass.hIcon         = icon;
    wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH) GetStockObject(NULL_BRUSH);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = NEBULA3_WINDOW_CLASS;
    wndClass.hIconSm       = NULL;
    RegisterClassEx(&wndClass);

    // we may need to adjust window size so that the client area of 
    // the window is of the requested size
    DWORD windowStyle = this->windowedStyle;
    if (0 != this->parentHwnd)
    {
        windowStyle = this->childWindowStyle;
    }
    else if (this->fullscreen)
    {
        windowStyle = this->fullscreenStyle;
    }
    DisplayMode adjMode = this->ComputeAdjustedWindowRect();

    // open window
    this->hWnd = CreateWindowEx(WS_EX_APPWINDOW,					// dwExStyle
							  NEBULA3_WINDOW_CLASS,                 // lpClassName
                              this->windowTitle.AsCharPtr(),        // lpWindowName
                              windowStyle,                          // dwStyle
                              adjMode.GetXPos(),                    // x
                              adjMode.GetYPos(),                    // y
                              adjMode.GetWidth(),                   // nWidth
                              adjMode.GetHeight(),                  // nHeight
                              NULL,				                    // hWndParent
                              NULL,                                 // hMenu
                              this->hInst,                          // hInstance
                              NULL);                                // lParam

    n_assert(0 != this->hWnd);

    // set topmost flag
    if (this->IsAlwaysOnTop())
    {
        SetWindowPos(this->hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    }

    // if we're in child-window mode, adjust the actually used display mode!
    if (0 != this->parentHwnd)
    {
        this->displayMode.SetWidth(adjMode.GetWidth());
        this->displayMode.SetHeight(adjMode.GetHeight());
        this->displayMode.SetXPos(adjMode.GetXPos());
        this->displayMode.SetYPos(adjMode.GetYPos());
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
Win32DisplayDevice::EmbedWindow()
{
	n_assert(0 != this->hInst);
	n_assert(0 != this->windowData.GetPtr());
	n_assert(0 == this->hWnd);	

	// embeds display device into an existing window by setting the hwnd
	ParentWindow* parentWindow = (ParentWindow*)this->windowData.GetPtr();
	this->parentHwnd = parentWindow->window;
	this->displayMode.SetWidth(parentWindow->width);
	this->displayMode.SetHeight(parentWindow->height);

	// now open window normally
	bool result = this->OpenWindow();

	// set parent
	SetParent(this->hWnd, this->parentHwnd);

	return result;
}

//------------------------------------------------------------------------------
/**
    Close the application window.
*/
void
Win32DisplayDevice::CloseWindow()
{
    n_assert(0 != this->hInst);

    // close the window (if not already happened), the window may
    // have been closed externally by Alt-F4 (for instance)
    if (0 != this->hWnd)
    {
        DestroyWindow(this->hWnd);
        this->hWnd = 0;
    }

    // release accelerator table
    if (this->hAccel) 
    {
        DestroyAcceleratorTable(this->hAccel);
        this->hAccel = 0;
    }

    // unregister the window class
    UnregisterClass(NEBULA3_WINDOW_CLASS, this->hInst);
}

//------------------------------------------------------------------------------
/**
    This will return an adjusted window size which takes the client
    area of the window into account. This is only relevant for windowed mode.
*/
DisplayMode
Win32DisplayDevice::ComputeAdjustedWindowRect()
{
    if (0 != this->parentHwnd)
    {
        RECT r = { 0 };
        GetClientRect(this->parentHwnd, &r);        
        AdjustWindowRect(&r, this->childWindowStyle, 0);
        return DisplayMode(0, 0, r.right - r.left, r.bottom - r.top, this->displayMode.GetPixelFormat());
    }
    else if (this->fullscreen)
    {
        return this->displayMode;
    }
    else
    {
        const DisplayMode& mode = this->displayMode;
        RECT r;
        r.left   = mode.GetXPos();
        r.right  = mode.GetXPos() + mode.GetWidth();
        r.top    = mode.GetYPos();
        r.bottom = mode.GetYPos() + mode.GetHeight();
        AdjustWindowRect(&r, this->windowedStyle, 0);
        return DisplayMode(mode.GetXPos(), mode.GetYPos(), r.right - r.left, r.bottom - r.top, mode.GetPixelFormat());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnMinimized()
{
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::DisplayMinimized));
    ReleaseCapture();
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnRestored()
{
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::DisplayRestored));
    // as a child window, do not release capture, because it would block
    // the resizing
    if (!this->parentHwnd)
    {
        ReleaseCapture();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
Win32DisplayDevice::OnSetCursor()
{
    return this->NotifyEventHandlers(DisplayEvent(DisplayEvent::SetCursor));
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnPaint()
{
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::Paint));
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnSetFocus()
{
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::SetFocus));
    ReleaseCapture();
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnKillFocus()
{
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::KillFocus));
    ReleaseCapture();
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnCloseRequested()
{
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::CloseRequested));
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnToggleFullscreenWindowed()
{
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::ToggleFullscreenWindowed));
}

//------------------------------------------------------------------------------
/**
*/
void 
Win32DisplayDevice::OnResize( WORD newWidth, WORD newHeight )
{
	this->NotifyEventHandlers(DisplayEvent(DisplayEvent::DisplayResized));
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnKeyDown(LPARAM lParam, WPARAM wParam)
{
    Input::Key::Code keyCode = this->TranslateKeyCode(lParam, wParam);
    if (Input::Key::InvalidKey != keyCode)
    {
        this->NotifyEventHandlers(DisplayEvent(DisplayEvent::KeyDown, keyCode));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnKeyUp(LPARAM lParam, WPARAM wParam)
{
    Input::Key::Code keyCode = this->TranslateKeyCode(lParam, wParam);
    if (Input::Key::InvalidKey != keyCode)
    {
        this->NotifyEventHandlers(DisplayEvent(DisplayEvent::KeyUp, keyCode));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnChar(WPARAM wParam)
{
    Input::Char chr = (Input::Char) wParam;
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::Character, chr));
}

//------------------------------------------------------------------------------
/**    
*/
float2
Win32DisplayDevice::ComputeAbsMousePos(LPARAM lParam) const
{
    return float2(float(short(LOWORD(lParam))), float(short(HIWORD(lParam))));
}

//------------------------------------------------------------------------------
/**    
*/
float2
Win32DisplayDevice::ComputeNormMousePos(const float2& absMousePos) const
{
    float2 normMousePos;
    RECT clientRect = { 0 };
    if ((0 != this->hWnd) && GetClientRect(this->hWnd, &clientRect))
    {
        LONG w = n_max(clientRect.right - clientRect.left, 1);
        LONG h = n_max(clientRect.bottom - clientRect.top, 1);
        normMousePos.set(absMousePos.x() / float(w), absMousePos.y() / float(h));
    }
    else
    {
        normMousePos.set(absMousePos.x() / float(this->displayMode.GetWidth()), absMousePos.y() / float(this->displayMode.GetHeight()));
    }
    return normMousePos;
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnMouseButton(UINT uMsg, LPARAM lParam)
{
    float2 absMousePos = this->ComputeAbsMousePos(lParam);
    float2 normMousePos = this->ComputeNormMousePos(absMousePos);
    switch (uMsg)
    {
        case WM_LBUTTONDBLCLK:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonDoubleClick, Input::MouseButton::LeftButton, absMousePos, normMousePos));
            break;

        case WM_RBUTTONDBLCLK:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonDoubleClick, Input::MouseButton::RightButton, absMousePos, normMousePos));
            break;

        case WM_MBUTTONDBLCLK:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonDoubleClick, Input::MouseButton::MiddleButton, absMousePos, normMousePos));
            break;

        case WM_LBUTTONDOWN:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonDown, Input::MouseButton::LeftButton, absMousePos, normMousePos));
            SetCapture(this->hWnd);
            break;

        case WM_RBUTTONDOWN:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonDown, Input::MouseButton::RightButton, absMousePos, normMousePos));
            SetCapture(this->hWnd);
            break;

        case WM_MBUTTONDOWN:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonDown, Input::MouseButton::MiddleButton, absMousePos, normMousePos));
            SetCapture(this->hWnd);
            break;

        case WM_LBUTTONUP:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonUp, Input::MouseButton::LeftButton, absMousePos, normMousePos));
            ReleaseCapture();
            break;

        case WM_RBUTTONUP:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonUp, Input::MouseButton::RightButton, absMousePos, normMousePos));
            ReleaseCapture();
            break;

        case WM_MBUTTONUP:
            this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseButtonUp, Input::MouseButton::MiddleButton, absMousePos, normMousePos));
            ReleaseCapture();
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnMouseMove(LPARAM lParam)
{
    float2 absMousePos = this->ComputeAbsMousePos(lParam);
    float2 normMousePos = this->ComputeNormMousePos(absMousePos);
    this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseMove, absMousePos, normMousePos));
}

//------------------------------------------------------------------------------
/**
*/
void
Win32DisplayDevice::OnMouseWheel(WPARAM wParam)
{
    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (zDelta > 0)
    {
        this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseWheelForward));
    }
    else
    {
        this->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseWheelBackward));
    }
}

//------------------------------------------------------------------------------
/**
    The Nebula3 WinProc.
*/
LRESULT CALLBACK
Win32DisplayDevice::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Win32DisplayDevice* self = Win32DisplayDevice::Instance();    
    switch (uMsg)
    {
        case WM_SYSCOMMAND:
            // prevent moving/sizing and power loss in fullscreen mode
            if (self->IsFullscreen())
            {
                switch (wParam)
                {
                    case SC_MOVE:
                    case SC_SIZE:
                    case SC_MAXIMIZE:
                    case SC_KEYMENU:
                    case SC_MONITORPOWER:
                        return 1;
                        break;
                }
            }
            break;

        case WM_ERASEBKGND:
            // prevent Windows from erasing the background
            return 1;

        case WM_SIZE:
            {
                // inform input server about focus change
                if ((SIZE_MAXHIDE == wParam) || (SIZE_MINIMIZED == wParam))
                {
                    self->OnMinimized();
                }
                else if (SIZE_RESTORED == wParam)
                {
                    self->OnRestored();					
                }

				// manually change window size in child mode
				WORD newWidth = LOWORD(lParam);
				WORD newHeight = HIWORD(lParam);
				self->OnResize(newWidth, newHeight); 
            }
            break;

        case WM_SETCURSOR:
            if (self->OnSetCursor())
            {
                return TRUE;
            }
            break;

        case WM_PAINT:
            self->OnPaint();
            break;

        case WM_SETFOCUS:
            self->OnSetFocus();
            break;

        case WM_KILLFOCUS:
            self->OnKillFocus();
            break;

        case WM_CLOSE:
            self->OnCloseRequested();
            self->hWnd = 0;
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == AccelToggleFullscreen)
            {
                self->OnToggleFullscreenWindowed();
            }
            break;

		case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            self->OnKeyDown(lParam, wParam);
            break;

		case WM_SYSKEYUP:
        case WM_KEYUP:
            self->OnKeyUp(lParam, wParam);
            break;

        case WM_CHAR:
            self->OnChar(wParam);
            break;

        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            self->OnMouseButton(uMsg, lParam);
            break;

        case WM_MOUSEMOVE:
            self->OnMouseMove(lParam);
            break;

        case WM_MOUSEWHEEL:
            self->OnMouseWheel(wParam);
            break;
    }

	return DefWindowProc(hWnd, uMsg, wParam, lParam); 
}

//------------------------------------------------------------------------------
/**
    Helper method which translates a Win32 virtual key code into a Nebula
    key code.
*/
Input::Key::Code
Win32DisplayDevice::TranslateKeyCode(LPARAM lParam, WPARAM wParam) const
{
    switch (wParam)
    {
        case VK_BACK:                   return Input::Key::Back;
        case VK_TAB:                    return Input::Key::Tab;
        case VK_CLEAR:                  return Input::Key::Clear;
        case VK_RETURN:                 return Input::Key::Return;
        case VK_SHIFT:                  
			{
				if (lParam & 0x01000000) return Input::Key::RightShift;
				else					 return Input::Key::LeftShift;
			}
        case VK_CONTROL:
			{
				if (lParam & 0x01000000) return Input::Key::RightControl;
				else					 return Input::Key::LeftControl;
			}
        case VK_MENU:                   
			{
				if (lParam & 0x01000000) return Input::Key::RightMenu;
				else					 return Input::Key::LeftMenu;
			}
        case VK_PAUSE:                  return Input::Key::Pause;
        case VK_CAPITAL:                return Input::Key::Capital;
        case VK_ESCAPE:                 return Input::Key::Escape;
        case VK_CONVERT:                return Input::Key::Convert;
        case VK_NONCONVERT:             return Input::Key::NonConvert;
        case VK_ACCEPT:                 return Input::Key::Accept;
        case VK_MODECHANGE:             return Input::Key::ModeChange;
        case VK_SPACE:                  return Input::Key::Space;
        case VK_PRIOR:                  return Input::Key::Prior;
        case VK_NEXT:                   return Input::Key::Next;
        case VK_END:                    return Input::Key::End;
        case VK_HOME:                   return Input::Key::Home;
        case VK_LEFT:                   return Input::Key::Left;
        case VK_RIGHT:                  return Input::Key::Right;
        case VK_UP:                     return Input::Key::Up;
        case VK_DOWN:                   return Input::Key::Down;
        case VK_SELECT:                 return Input::Key::Select;
        case VK_PRINT:                  return Input::Key::Print;
        case VK_EXECUTE:                return Input::Key::Execute;
        case VK_SNAPSHOT:               return Input::Key::Snapshot;
        case VK_INSERT:                 return Input::Key::Insert;
        case VK_DELETE:                 return Input::Key::Delete;
        case VK_HELP:                   return Input::Key::Help;
        case VK_LWIN:                   return Input::Key::LeftWindows;
        case VK_RWIN:                   return Input::Key::RightWindows;
        case VK_APPS:                   return Input::Key::Apps;
        case VK_SLEEP:                  return Input::Key::Sleep;
        case VK_NUMPAD0:                return Input::Key::NumPad0;
        case VK_NUMPAD1:                return Input::Key::NumPad1;
        case VK_NUMPAD2:                return Input::Key::NumPad2;
        case VK_NUMPAD3:                return Input::Key::NumPad3;
        case VK_NUMPAD4:                return Input::Key::NumPad4;
        case VK_NUMPAD5:                return Input::Key::NumPad5;
        case VK_NUMPAD6:                return Input::Key::NumPad6;
        case VK_NUMPAD7:                return Input::Key::NumPad7;
        case VK_NUMPAD8:                return Input::Key::NumPad8;
        case VK_NUMPAD9:                return Input::Key::NumPad9;
        case VK_MULTIPLY:               return Input::Key::Multiply;
        case VK_ADD:                    return Input::Key::Add;
		case VK_SUBTRACT:				return Input::Key::Subtract;
		case VK_OEM_COMMA:				return Input::Key::Comma;
		case VK_OEM_PERIOD:				return Input::Key::Period;
        case VK_SEPARATOR:              return Input::Key::Separator;
        case VK_DECIMAL:                return Input::Key::Decimal;
        case VK_DIVIDE:                 return Input::Key::Divide;
        case VK_F1:                     return Input::Key::F1;
        case VK_F2:                     return Input::Key::F2;
        case VK_F3:                     return Input::Key::F3;
        case VK_F4:                     return Input::Key::F4;
        case VK_F5:                     return Input::Key::F5;
        case VK_F6:                     return Input::Key::F6;
        case VK_F7:                     return Input::Key::F7;
        case VK_F8:                     return Input::Key::F8;
        case VK_F9:                     return Input::Key::F9;
        case VK_F10:                    return Input::Key::F10;
        case VK_F11:                    return Input::Key::F11;
        case VK_F12:                    return Input::Key::F12;
        case VK_F13:                    return Input::Key::F13;
        case VK_F14:                    return Input::Key::F14;
        case VK_F15:                    return Input::Key::F15;
        case VK_F16:                    return Input::Key::F16;
        case VK_F17:                    return Input::Key::F17;
        case VK_F18:                    return Input::Key::F18;
        case VK_F19:                    return Input::Key::F19;
        case VK_F20:                    return Input::Key::F20;
        case VK_F21:                    return Input::Key::F21;
        case VK_F22:                    return Input::Key::F22;
        case VK_F23:                    return Input::Key::F23;
        case VK_F24:                    return Input::Key::F24;
        case VK_NUMLOCK:                return Input::Key::NumLock;
        case VK_SCROLL:                 return Input::Key::Scroll;
        case VK_BROWSER_BACK:           return Input::Key::BrowserBack;
        case VK_BROWSER_FORWARD:        return Input::Key::BrowserForward;
        case VK_BROWSER_REFRESH:        return Input::Key::BrowserRefresh;
        case VK_BROWSER_STOP:           return Input::Key::BrowserStop;
        case VK_BROWSER_SEARCH:         return Input::Key::BrowserSearch;
        case VK_BROWSER_FAVORITES:      return Input::Key::BrowserFavorites;
        case VK_BROWSER_HOME:           return Input::Key::BrowserHome;
        case VK_VOLUME_MUTE:            return Input::Key::VolumeMute;
        case VK_VOLUME_DOWN:            return Input::Key::VolumeDown;
        case VK_VOLUME_UP:              return Input::Key::VolumeUp;
        case VK_MEDIA_NEXT_TRACK:       return Input::Key::MediaNextTrack;
        case VK_MEDIA_PREV_TRACK:       return Input::Key::MediaPrevTrack;
        case VK_MEDIA_STOP:             return Input::Key::MediaStop;
        case VK_MEDIA_PLAY_PAUSE:       return Input::Key::MediaPlayPause;
        case VK_LAUNCH_MAIL:            return Input::Key::LaunchMail;
        case VK_LAUNCH_MEDIA_SELECT:    return Input::Key::LaunchMediaSelect;
        case VK_LAUNCH_APP1:            return Input::Key::LaunchApp1;
        case VK_LAUNCH_APP2:            return Input::Key::LaunchApp2;
		case VK_OEM_3:					return Input::Key::Tilde;
        case '0':                       return Input::Key::Key0;
        case '1':                       return Input::Key::Key1;
        case '2':                       return Input::Key::Key2;
        case '3':                       return Input::Key::Key3;
        case '4':                       return Input::Key::Key4;
        case '5':                       return Input::Key::Key5;
        case '6':                       return Input::Key::Key6;
        case '7':                       return Input::Key::Key7;
        case '8':                       return Input::Key::Key8;
        case '9':                       return Input::Key::Key9;
        case 'A':                       return Input::Key::A;
        case 'B':                       return Input::Key::B;
        case 'C':                       return Input::Key::C;
        case 'D':                       return Input::Key::D;
        case 'E':                       return Input::Key::E;
        case 'F':                       return Input::Key::F;
        case 'G':                       return Input::Key::G;
        case 'H':                       return Input::Key::H;
        case 'I':                       return Input::Key::I;
        case 'J':                       return Input::Key::J;
        case 'K':                       return Input::Key::K;
        case 'L':                       return Input::Key::L;
        case 'M':                       return Input::Key::M;
        case 'N':                       return Input::Key::N;
        case 'O':                       return Input::Key::O;
        case 'P':                       return Input::Key::P;
        case 'Q':                       return Input::Key::Q;
        case 'R':                       return Input::Key::R;
        case 'S':                       return Input::Key::S;
        case 'T':                       return Input::Key::T;
        case 'U':                       return Input::Key::U;
        case 'V':                       return Input::Key::V;
        case 'W':                       return Input::Key::W;
        case 'X':                       return Input::Key::X;
        case 'Y':                       return Input::Key::Y;
        case 'Z':                       return Input::Key::Z;
        default:                        return Input::Key::InvalidKey;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Win32DisplayDevice::PostEvent( const Win32::Win32Event& winEvent )
{
	// propagates windows event to winproc 
	// WinProc(winEvent.GetHWND(), winEvent.GetMessage(), winEvent.GetWParam(), winEvent.GetLParam());
}

} // namespace CoreGraphics