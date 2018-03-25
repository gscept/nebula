//------------------------------------------------------------------------------
//	glfwdisplaydevice.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "render/stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/glfw/glfwdisplaydevice.h"
#if __OGL4__
#include "coregraphics/ogl4/ogl4types.h"
#endif
#include "coregraphics/renderdevice.h"
#include "GLFW/glfw3native.h"
#include <GLFW/glfw3.h>

#if __WIN32__

// Forward-declare GLFW windowProc
static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif


#if NEBULA3_OPENGL4_DEBUG
//------------------------------------------------------------------------------
/**
*/
void
NebulaGLFWErrorCallback(int errcode, const char* msg)
{
	n_error("GL ERROR: code %d, %s", errcode, msg);
}

#endif


namespace GLFW
{
__ImplementClass(GLFW::GLFWDisplayDevice, 'O4WD', Base::DisplayDeviceBase);
__ImplementSingleton(GLFW::GLFWDisplayDevice);

using namespace Util;
using namespace Math;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
GLFWDisplayDevice::GLFWDisplayDevice()
{
    __ConstructSingleton;
	glfwInit();	
}

//------------------------------------------------------------------------------
/**
*/
GLFWDisplayDevice::~GLFWDisplayDevice()
{
    __DestructSingleton;
	glfwTerminate();
}

//------------------------------------------------------------------------------
/**
    Open the display.
*/
bool
GLFWDisplayDevice::Open()
{
    n_assert(!this->IsOpen());

	if (DisplayDeviceBase::Open())
	{
		// setup event handler for input when we created the display
		this->eventHandler = GLFWInputDisplayEventHandler::Create();
		this->AttachEventHandler(this->eventHandler.upcast<DisplayEventHandler>());

		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
    Close the display.
*/
void
GLFWDisplayDevice::Close()
{
	n_assert(this->IsOpen());
	this->RemoveEventHandler(this->eventHandler.upcast<DisplayEventHandler>());
	this->eventHandler = nullptr;
	DisplayDeviceBase::Close();
}

//------------------------------------------------------------------------------
/**
    Process window system messages. Override this method in a subclass.
*/
void
GLFWDisplayDevice::ProcessWindowMessages()
{
	glfwPollEvents();
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWDisplayDevice::SetVerticalSyncEnabled(bool b)
{
	DisplayDeviceBase::SetVerticalSyncEnabled(b);
	glfwSwapInterval(b ? 1 : 0);
}

//------------------------------------------------------------------------------
/**
*/
bool
MatchPixelFormat(PixelFormat::Code format, const GLFWvidmode & mode)
{
	//FIXME this is just a subset
	switch(format)
	{
		case PixelFormat::R8G8B8X8:
		case PixelFormat::R8G8B8A8:
		case PixelFormat::SRGBA8:
		case PixelFormat::R8G8B8:
			return (mode.blueBits == 8) && (mode.redBits == 8) && (mode.greenBits == 8);
		break;
		case PixelFormat::R5G6B5:
			return (mode.blueBits == 5) && (mode.redBits == 5) && (mode.greenBits == 6);
		default:
			return false;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
    Returns the display modes on the given adapter in the given pixel format.
*/
Util::Array<DisplayMode>
GLFWDisplayDevice::GetAvailableDisplayModes(Adapter::Code adapter, PixelFormat::Code pixelFormat)
{    
    Util::Array<DisplayMode> ret;
	GLFWmonitor* monitor = GetMonitor(adapter);
	
	if (monitor)
	{
		int count;
		const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);
		for (int i = 0; i < count; i++)
		{
			if (MatchPixelFormat(pixelFormat, modes[i]))
			{
				DisplayMode mode(modes->width, modes->height, pixelFormat);
				ret.Append(mode);
			}
		}	
	}
    return ret;
}

//------------------------------------------------------------------------------
/**
    This method checks the available display modes on the given adapter
    against the requested display modes and returns true if the display mode
    exists.
*/
bool
GLFWDisplayDevice::SupportsDisplayMode(Adapter::Code adapter, const DisplayMode& requestedMode)
{	
	//FIXME only checks for width/height
	Util::Array<DisplayMode> modes = GetAvailableDisplayModes(adapter, requestedMode.GetPixelFormat());
	for (Util::Array<DisplayMode>::Iterator iter = modes.Begin(); iter != modes.End(); iter++)
	{
		if ((requestedMode.GetHeight() == iter->GetHeight()) && (requestedMode.GetWidth() == iter->GetWidth()))
			return true;
	}
	return false;
}


//------------------------------------------------------------------------------
/**
*/
GLFWmonitor*
GLFWDisplayDevice::GetMonitor(int index)
{
	GLFWmonitor* monitor = NULL;
    switch(adapter)
	{
		case CoreGraphics::Adapter::Primary:
			{
				return glfwGetPrimaryMonitor();
			}
		break;
		default:
			{
				// grab one of the others
				int count;
				GLFWmonitor** monitors = glfwGetMonitors(&count);
				n_assert(count > 1);
				// glfw stores primary in first element
				monitor = monitors[index];
			}			
		break;
	}
	return monitor;
}

//------------------------------------------------------------------------------
/**
    This method returns the current adapter display mode. It can be used
    to get the current desktop display mode.
*/
DisplayMode
GLFWDisplayDevice::GetCurrentAdapterDisplayMode(Adapter::Code adapter)
{
    GLFWmonitor* monitor = GetMonitor(adapter);
	n_assert(monitor);
	const GLFWvidmode * mode = glfwGetVideoMode(monitor);
	PixelFormat::Code format;
	//FIXME
	if ((mode->greenBits == 8) && (mode->blueBits == 8) && (mode->redBits == 8))
	{
		format = PixelFormat::A8B8G8R8;
	}
	else
	{
		format = PixelFormat::InvalidPixelFormat;
	}
	
	DisplayMode dmode(mode->width, mode->height, format);
	dmode.SetRefreshRate(mode->refreshRate);
	return dmode;
}

//------------------------------------------------------------------------------
/**
    Checks if the given adapter exists.
*/
bool
GLFWDisplayDevice::AdapterExists(Adapter::Code adapter)
{
    return NULL != GetMonitor(adapter);
}

//------------------------------------------------------------------------------
/**
    Returns information about the provided adapter.
*/
AdapterInfo
GLFWDisplayDevice::GetAdapterInfo(Adapter::Code adapter)
{
    AdapterInfo emptyAdapterInfo;
    return emptyAdapterInfo;
}

//------------------------------------------------------------------------------
/**
    translate keycode
*/
Input::Key::Code 
GLFWDisplayDevice::TranslateKeyCode(int inkey)
{
	switch (inkey)
	{
	case GLFW_KEY_BACKSPACE:                   return Input::Key::Back;
	case GLFW_KEY_TAB:                    return Input::Key::Tab;	
	case GLFW_KEY_ENTER:                 return Input::Key::Return;			
	case GLFW_KEY_MENU:                   return Input::Key::Menu;
	case GLFW_KEY_PAUSE:                  return Input::Key::Pause;
	case GLFW_KEY_CAPS_LOCK:                return Input::Key::Capital;
	case GLFW_KEY_ESCAPE:                 return Input::Key::Escape;				
	case GLFW_KEY_SPACE:                  return Input::Key::Space;		
	case GLFW_KEY_END:                    return Input::Key::End;
	case GLFW_KEY_HOME:                   return Input::Key::Home;
	case GLFW_KEY_LEFT:                   return Input::Key::Left;
	case GLFW_KEY_RIGHT:                  return Input::Key::Right;
	case GLFW_KEY_UP:                     return Input::Key::Up;
	case GLFW_KEY_DOWN:                   return Input::Key::Down;	
	case GLFW_KEY_INSERT:                 return Input::Key::Insert;
	case GLFW_KEY_DELETE:                 return Input::Key::Delete;	
	case GLFW_KEY_LEFT_SUPER:                   return Input::Key::LeftWindows;
	case GLFW_KEY_RIGHT_SUPER:                   return Input::Key::RightWindows;	
	case GLFW_KEY_KP_0:                return Input::Key::NumPad0;
	case GLFW_KEY_KP_1:                return Input::Key::NumPad1;
	case GLFW_KEY_KP_2:                return Input::Key::NumPad2;
	case GLFW_KEY_KP_3:                return Input::Key::NumPad3;
	case GLFW_KEY_KP_4:                return Input::Key::NumPad4;
	case GLFW_KEY_KP_5:                return Input::Key::NumPad5;
	case GLFW_KEY_KP_6:                return Input::Key::NumPad6;
	case GLFW_KEY_KP_7:                return Input::Key::NumPad7;
	case GLFW_KEY_KP_8:                return Input::Key::NumPad8;
	case GLFW_KEY_KP_9:                return Input::Key::NumPad9;
	case GLFW_KEY_KP_MULTIPLY:               return Input::Key::Multiply;
	case GLFW_KEY_KP_ADD:                    return Input::Key::Add;
	case GLFW_KEY_KP_SUBTRACT:				return Input::Key::Subtract;
	case GLFW_KEY_COMMA:				return Input::Key::Comma;
	case GLFW_KEY_PERIOD:				return Input::Key::Period;
	case GLFW_KEY_APOSTROPHE:              return Input::Key::Separator;
	case GLFW_KEY_KP_DECIMAL:                return Input::Key::Decimal;
	case GLFW_KEY_KP_DIVIDE:                 return Input::Key::Divide;
	case GLFW_KEY_F1:                     return Input::Key::F1;
	case GLFW_KEY_F2:                     return Input::Key::F2;
	case GLFW_KEY_F3:                     return Input::Key::F3;
	case GLFW_KEY_F4:                     return Input::Key::F4;
	case GLFW_KEY_F5:                     return Input::Key::F5;
	case GLFW_KEY_F6:                     return Input::Key::F6;
	case GLFW_KEY_F7:                     return Input::Key::F7;
	case GLFW_KEY_F8:                     return Input::Key::F8;
	case GLFW_KEY_F9:                     return Input::Key::F9;
	case GLFW_KEY_F10:                    return Input::Key::F10;
	case GLFW_KEY_F11:                    return Input::Key::F11;
	case GLFW_KEY_F12:                    return Input::Key::F12;
	case GLFW_KEY_F13:                    return Input::Key::F13;
	case GLFW_KEY_F14:                    return Input::Key::F14;
	case GLFW_KEY_F15:                    return Input::Key::F15;
	case GLFW_KEY_F16:                    return Input::Key::F16;
	case GLFW_KEY_F17:                    return Input::Key::F17;
	case GLFW_KEY_F18:                    return Input::Key::F18;
	case GLFW_KEY_F19:                    return Input::Key::F19;
	case GLFW_KEY_F20:                    return Input::Key::F20;
	case GLFW_KEY_F21:                    return Input::Key::F21;
	case GLFW_KEY_F22:                    return Input::Key::F22;
	case GLFW_KEY_F23:                    return Input::Key::F23;
	case GLFW_KEY_F24:                    return Input::Key::F24;
	case GLFW_KEY_NUM_LOCK:                return Input::Key::NumLock;
	case GLFW_KEY_SCROLL_LOCK:                 return Input::Key::Scroll;
	case GLFW_KEY_LEFT_SHIFT:                 return Input::Key::LeftShift;
	case GLFW_KEY_RIGHT_SHIFT:                 return Input::Key::RightShift;
	case GLFW_KEY_LEFT_CONTROL:               return Input::Key::LeftControl;
	case GLFW_KEY_RIGHT_CONTROL:               return Input::Key::RightControl;
	case GLFW_KEY_LEFT_ALT:                  return Input::Key::LeftMenu;
	case GLFW_KEY_RIGHT_ALT:                  return Input::Key::RightMenu;	
	//case VK_OEM_3:					return Input::Key::Tilde;
	case GLFW_KEY_0:                       return Input::Key::Key0;
	case GLFW_KEY_1:                       return Input::Key::Key1;
	case GLFW_KEY_2:                       return Input::Key::Key2;
	case GLFW_KEY_3:                       return Input::Key::Key3;
	case GLFW_KEY_4:                       return Input::Key::Key4;
	case GLFW_KEY_5:                       return Input::Key::Key5;
	case GLFW_KEY_6:                       return Input::Key::Key6;
	case GLFW_KEY_7:                       return Input::Key::Key7;
	case GLFW_KEY_8:                       return Input::Key::Key8;
	case GLFW_KEY_9:                       return Input::Key::Key9;
	case GLFW_KEY_A:                       return Input::Key::A;
	case GLFW_KEY_B:                       return Input::Key::B;
	case GLFW_KEY_C:                       return Input::Key::C;
	case GLFW_KEY_D:                       return Input::Key::D;
	case GLFW_KEY_E:                       return Input::Key::E;
	case GLFW_KEY_F:                       return Input::Key::F;
	case GLFW_KEY_G:                       return Input::Key::G;
	case GLFW_KEY_H:                       return Input::Key::H;
	case GLFW_KEY_I:                       return Input::Key::I;
	case GLFW_KEY_J:                       return Input::Key::J;
	case GLFW_KEY_K:                       return Input::Key::K;
	case GLFW_KEY_L:                       return Input::Key::L;
	case GLFW_KEY_M:                       return Input::Key::M;
	case GLFW_KEY_N:                       return Input::Key::N;
	case GLFW_KEY_O:                       return Input::Key::O;
	case GLFW_KEY_P:                       return Input::Key::P;
	case GLFW_KEY_Q:                       return Input::Key::Q;
	case GLFW_KEY_R:                       return Input::Key::R;
	case GLFW_KEY_S:                       return Input::Key::S;
	case GLFW_KEY_T:                       return Input::Key::T;
	case GLFW_KEY_U:                       return Input::Key::U;
	case GLFW_KEY_V:                       return Input::Key::V;
	case GLFW_KEY_W:                       return Input::Key::W;
	case GLFW_KEY_X:                       return Input::Key::X;
	case GLFW_KEY_Y:                       return Input::Key::Y;
	case GLFW_KEY_Z:                       return Input::Key::Z;
	default:                        return Input::Key::InvalidKey;
	}
	return Input::Key::InvalidKey;
}


} // namespace GLFW

