#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::GLFWDisplayDevice

    GLFW based OpenGL implementation of DisplayDevice class.

    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "GLFW/glfw3.h"
#include "coregraphics/base/displaydevicebase.h"
#include "util/array.h"
#include "threading/thread.h"
#include "input/glfw/glfwinputdisplayeventhandler.h"

namespace GLFW
{
class GLFWDisplayDevice : public Base::DisplayDeviceBase
{
    __DeclareClass(GLFWDisplayDevice);
    __DeclareSingleton(GLFWDisplayDevice);
public:
    /// constructor
    GLFWDisplayDevice();
    /// destructor
    virtual ~GLFWDisplayDevice();

	/// open the display
	bool Open();
	/// close the display
	void Close();
	/// process window system messages, call this method once per frame
	static void ProcessWindowMessages();

	/// set if vertical sync should be used
	void SetVerticalSyncEnabled(bool b);

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

	/// translate glfw keycodes to nebula ones
	static Input::Key::Code TranslateKeyCode(int inkey);

protected:
	Ptr<GLFWInputDisplayEventHandler> eventHandler;
	friend class GLFWWindow;

	friend class OGL4RenderDevice;
    friend class GLFWInputServer;

	friend void CoreGraphics::DestroyWindow(const WindowId id);
	friend void KeyFunc(const CoreGraphics::WindowId& id, int key, int scancode, int action, int mods);
	friend void CharFunc(const CoreGraphics::WindowId& id, unsigned int key);
	friend void MouseButtonFunc(const CoreGraphics::WindowId& id, int button, int action, int mods);
	friend void MouseFunc(const CoreGraphics::WindowId& id, double xpos, double ypos);
	friend void ScrollFunc(const CoreGraphics::WindowId& id, double xs, double ys);
	friend void CloseFunc(const CoreGraphics::WindowId& id);
	friend void FocusFunc(const CoreGraphics::WindowId& id, int focus);
	friend void ResizeFunc(const CoreGraphics::WindowId& id, int width, int height);
	friend const CoreGraphics::WindowId InternalSetupFunction(const CoreGraphics::WindowCreateInfo& info, const Util::Blob& windowData, bool embed);

	/// retrieve monitor from adapter. can be NULL
	GLFWmonitor* GetMonitor(int index);

};

} // namespace GLFW
//------------------------------------------------------------------------------
