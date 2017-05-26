//------------------------------------------------------------------------------
// glfwwindow.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "glfwwindow.h"
#include "glfwdisplaydevice.h"
#include "input/key.h"
#include "coregraphics/displayevent.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/displaydevice.h"

using namespace CoreGraphics;
using namespace Math;
namespace GLFW
{

__ImplementClass(GLFW::GLFWWindow, 'GFWW', Base::WindowBase);
//------------------------------------------------------------------------------
/**
*/
GLFWWindow::GLFWWindow() :
	window(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
GLFWWindow::~GLFWWindow()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::Open()
{
#if __OGL4__
#if NEBULA3_OPENGL4_DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwSetErrorCallback(NebulaGLFWErrorCallback);
	n_printf("Creating OpenGL debug context\n");
#else
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);
	glDisable(GL_DEBUG_OUTPUT);
#endif
#endif
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

	// get original window, if this is the first window, then the parent window will simply be nullptr
	GLFWwindow* wnd = NULL;
	const Ptr<CoreGraphics::Window>& origWindow = CoreGraphics::DisplayDevice::Instance()->GetMainWindow();	
	if (origWindow.isvalid()) wnd = origWindow->window;

#if __VULKAN__
	// if Vulkan, context is created and managed by render device
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

	// if we have window data, setup from alien window
	if (this->windowData.Size() > 0)
	{
		// create window using our Qt window as child
		this->window = glfwCreateWindowFromAlien(this->windowData.GetPtr(), wnd);
		//if (wnd != NULL) glfwCopyContext(origWindow->window, this->window);
		glfwMakeContextCurrent(this->window);

		// get actual window size
		int height, width;
		glfwGetWindowSize(this->window, &width, &height);

		// update display mode
		this->displayMode.SetWidth(width);
		this->displayMode.SetHeight(height);
		this->displayMode.SetAspectRatio(width / float(height));

		// set user pointer to this window
		glfwSetWindowUserPointer(this->window, this);
	}
	else
	{
		glfwWindowHint(GLFW_RESIZABLE, this->resizable ? GL_TRUE : GL_FALSE);
		glfwWindowHint(GLFW_DECORATED, this->decorated ? GL_TRUE : GL_FALSE);

		// create window
		this->window = glfwCreateWindow(this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->windowTitle.AsCharPtr(), NULL, wnd);
		if (!this->fullscreen) glfwSetWindowPos(this->window, this->displayMode.GetXPos(), this->displayMode.GetYPos());
		else				   this->ApplyFullscreen();
		
#if __OGL4__
		// make current if GL4
		glfwMakeContextCurrent(this->window);
#endif

		// set user pointer to this window
		glfwSetWindowUserPointer(this->window, this);
		glfwSetWindowTitle(this->window, this->windowTitle.AsCharPtr());
	}	

#if __VULKAN__
	VkResult res = glfwCreateWindowSurface(Vulkan::VkRenderDevice::instance, this->window, nullptr, &this->surface);
	n_assert(res == VK_SUCCESS);

	// setup swapchain
	this->SetupVulkanSwapchain();
#endif

	// notify window is opened
	GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowOpen, this->windowId));

	// enable callbacks
	this->EnableCallbacks();

#if __OGL4__
	// with OpenGL, we have only one context, so any extra windows will just have their own render target
	if (origWindow.isvalid() && origWindow != this)
	{
		glfwReparentContext(origWindow->window, this->window);
	}
#endif

	DisplayDevice::Instance()->MakeWindowCurrent(this->windowId);
	WindowBase::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::Close()
{
	this->DisableCallbacks();
	glfwDestroyWindow(this->window);

#if __VULKAN__
	// discard swapchain
	this->DiscardVulkanSwapchain();
#endif

	// close event
	GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowClose, this->windowId));
	WindowBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::Reopen()
{
	// just ignore full screen if this window is embedded
	if (!this->embedded)
	{
		// if we toggle full screen, select monitor (just selects primary for the moment) and apply full screen
		if (this->fullscreen)
		{
			this->ApplyFullscreen();
		}
		else
		{
			// if not, set the window state and detach from the monitor
			glfwSetWindowMonitor(this->window, NULL, this->displayMode.GetXPos(), this->displayMode.GetYPos(), this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->displayMode.GetRefreshRate());
		}
	}

	// only move window if not fullscreen
	if (!this->fullscreen)
	{
		// update window with new size and position
		glfwSetWindowPos(this->window, this->displayMode.GetXPos(), this->displayMode.GetYPos());
		glfwSetWindowSize(this->window, this->displayMode.GetWidth(), this->displayMode.GetHeight());
	}

	// post event
	GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowReopen, this->windowId));

	// open window again
	WindowBase::Reopen();
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::SetTitle(const Util::String& title)
{
	glfwSetWindowTitle(this->window, title.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::SetFullscreen(bool b, int monitor)
{
	WindowBase::SetFullscreen(b, monitor);
	if (this->window && b)
	{
		this->ApplyFullscreen();
	}
	else
	{
		glfwSetWindowMonitor(this->window, NULL, this->displayMode.GetXPos(), this->displayMode.GetYPos(), this->displayMode.GetWidth(), this->displayMode.GetHeight(), this->displayMode.GetRefreshRate());
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::SetCursorVisible(bool visible)
{
	WindowBase::SetCursorVisible(visible);
	glfwSetInputMode(this->window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::SetCursorLocked(bool locked)
{
	WindowBase::SetCursorLocked(locked);
	glfwSetInputMode(this->window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

//------------------------------------------------------------------------------
/**
callback for key events
*/
void
staticKeyFunc(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->KeyFunc(key, scancode, action, mods);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::KeyFunc(int key, int scancode, int action, int mods)
{
	Input::Key::Code keyCode = GLFWDisplayDevice::TranslateKeyCode(key);
	DisplayEvent::Code evtype;
	switch (action)
	{
	case GLFW_REPEAT:
	case GLFW_PRESS:
		evtype = DisplayEvent::KeyDown;
		break;
	case GLFW_RELEASE:
		evtype = DisplayEvent::KeyUp;
		break;
	default:
		return;
	}
	if (Input::Key::InvalidKey != keyCode)
	{
		GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(evtype, keyCode));
	}
}


//------------------------------------------------------------------------------
/**
callbacks for character input
*/
void
staticCharFunc(GLFWwindow* window, unsigned int key)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->CharFunc(key);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::CharFunc(unsigned int key)
{
	GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::Character, (Input::Char)key));
}

//------------------------------------------------------------------------------
/**
*/
void
staticMouseButtonFunc(GLFWwindow* window, int button, int action, int mods)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->MouseButtonFunc(button, action, mods);
}

//------------------------------------------------------------------------------
/**
callbacks for mouse buttons
*/
void
GLFWWindow::MouseButtonFunc(int button, int action, int mods)
{
	DisplayEvent::Code act = action == GLFW_PRESS ? DisplayEvent::MouseButtonDown : DisplayEvent::MouseButtonUp;
	Input::MouseButton::Code but;
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		but = Input::MouseButton::LeftButton;
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		but = Input::MouseButton::RightButton;
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		but = Input::MouseButton::MiddleButton;
		break;
	default:
		return;
	}
	double x, y;
	glfwGetCursorPos(this->window, &x, &y);

	float2 pos;
	pos.set((float)x / float(this->displayMode.GetWidth()), (float)y / float(this->displayMode.GetHeight()));
	GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(act, but, float2((float)x, (float)y), pos));
}

//------------------------------------------------------------------------------
/**
callbacks for mouse position
*/
void
staticMouseFunc(GLFWwindow* window, double xpos, double ypos)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->MouseFunc(xpos, ypos);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::MouseFunc(double xpos, double ypos)
{
	float2 absMousePos((float)xpos, (float)ypos);
	float2 pos;
	pos.set((float)xpos / float(this->displayMode.GetWidth()), (float)ypos / float(this->displayMode.GetHeight()));
	GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::MouseMove, absMousePos, pos));
}

//------------------------------------------------------------------------------
/**
callbacks for scroll event
*/
void
staticScrollFunc(GLFWwindow* window, double xs, double ys)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->ScrollFunc(xs, ys);
}

//------------------------------------------------------------------------------
/**
callback for scroll events. only vertical scrolling is supported
and no scroll amounts are used, just single steps
*/
void
GLFWWindow::ScrollFunc(double xs, double ys)
{
	if (ys != 0.0)
	{
		GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(ys > 0.0f ? DisplayEvent::MouseWheelForward : DisplayEvent::MouseWheelBackward));
	}
}

//------------------------------------------------------------------------------
/**
callback for close requested
*/
void
staticCloseFunc(GLFWwindow* window)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->CloseFunc();
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::CloseFunc()
{
	GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::CloseRequested, this->windowId));
}

//------------------------------------------------------------------------------
/**
callback for focus
*/
void
staticFocusFunc(GLFWwindow* window, int focus)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->FocusFunc(focus);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::FocusFunc(int focus)
{
	if (focus)
	{
		GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::SetFocus, this->windowId));
	}
	else
	{
		GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::KillFocus, this->windowId));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
staticSizeFunc(GLFWwindow* window, int width, int height)
{
	GLFWWindow* wnd = (GLFWWindow*)glfwGetWindowUserPointer(window);
	wnd->ResizeFunc(width, height);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::ResizeFunc(int width, int height)
{
	// only resize if size is not 0
	if (width != 0 && height != 0)
	{
		this->displayMode.SetWidth(width);
		this->displayMode.SetHeight(height);
		this->displayMode.SetAspectRatio(width / float(height));

		// resize default render target
		this->Resize(width, height);

		// notify event listeners we resized
		GLFWDisplayDevice::Instance()->NotifyEventHandlers(DisplayEvent(DisplayEvent::WindowResized, this->windowId));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
staticDropFunc(GLFWwindow* window, int files, const char** args)
{
	// empty for now
}

//------------------------------------------------------------------------------
/**
Enables callbacks through glfw
*/
void
GLFWWindow::EnableCallbacks()
{
	glfwSetKeyCallback(this->window, staticKeyFunc);
	glfwSetMouseButtonCallback(this->window, staticMouseButtonFunc);
	glfwSetCursorPosCallback(this->window, staticMouseFunc);
	glfwSetWindowCloseCallback(this->window, staticCloseFunc);
	glfwSetWindowFocusCallback(this->window, staticFocusFunc);
	glfwSetWindowSizeCallback(this->window, staticSizeFunc);
	glfwSetScrollCallback(this->window, staticScrollFunc);
	glfwSetCharCallback(this->window, staticCharFunc);
	glfwSetDropCallback(this->window, staticDropFunc);
}

//------------------------------------------------------------------------------
/**
Disables callbacks through glfw
*/
void
GLFWWindow::DisableCallbacks()
{
	glfwSetKeyCallback(this->window, NULL);
	glfwSetMouseButtonCallback(this->window, NULL);
	glfwSetCursorPosCallback(this->window, NULL);
	glfwSetWindowCloseCallback(this->window, NULL);
	glfwSetWindowFocusCallback(this->window, NULL);
	glfwSetWindowSizeCallback(this->window, NULL);
	glfwSetScrollCallback(this->window, NULL);
	glfwSetCharCallback(this->window, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::ApplyFullscreen()
{
	GLFWmonitor* mon;
	if (monitor == -1)
	{
		mon = glfwGetPrimaryMonitor();
	}
	else
	{
		int count;
		GLFWmonitor** monitors = glfwGetMonitors(&count);
		n_assert(monitor < count);
		mon = monitors[monitor];
	}
	const GLFWvidmode* mode = glfwGetVideoMode(mon);
	glfwSetWindowMonitor(this->window, mon, 0,0, mode->width, mode->height, 60);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::MakeCurrent()
{
#if __OGL4__
	glfwMakeContextCurrent(this->window);
#endif
}

#if __VULKAN__
using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::SetupVulkanSwapchain()
{
	// find available surface formats
	uint32_t numFormats;
	VkResult res;
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(VkRenderDevice::physicalDev, this->surface, &numFormats, NULL);
	n_assert(res == VK_SUCCESS);

	Util::FixedArray<VkSurfaceFormatKHR> formats(numFormats);
	res = vkGetPhysicalDeviceSurfaceFormatsKHR(VkRenderDevice::physicalDev, this->surface, &numFormats, formats.Begin());
	n_assert(res == VK_SUCCESS);
	this->format = formats[0].format;
	this->colorSpace = formats[0].colorSpace;
	VkComponentMapping mapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	uint32_t i;
	for (i = 0; i < numFormats; i++)
	{
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
		{
			this->format = formats[i].format;
			this->colorSpace = formats[i].colorSpace;
			mapping.r = VK_COMPONENT_SWIZZLE_R;
			mapping.b = VK_COMPONENT_SWIZZLE_B;
			break;
		}
	}

	// get surface capabilities
	VkSurfaceCapabilitiesKHR surfCaps;
	res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkRenderDevice::physicalDev, this->surface, &surfCaps);
	n_assert(res == VK_SUCCESS);

	uint32_t numPresentModes;
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(VkRenderDevice::physicalDev, this->surface, &numPresentModes, NULL);
	n_assert(res == VK_SUCCESS);

	// get present modes
	Util::FixedArray<VkPresentModeKHR> presentModes(numPresentModes);
	res = vkGetPhysicalDeviceSurfacePresentModesKHR(VkRenderDevice::physicalDev, this->surface, &numPresentModes, presentModes.Begin());
	n_assert(res == VK_SUCCESS);

	VkExtent2D swapchainExtent;
	if (surfCaps.currentExtent.width == -1)
	{
		const DisplayMode& mode = this->displayMode;
		swapchainExtent.width = mode.GetWidth();
		swapchainExtent.height = mode.GetHeight();
	}
	else
	{
		swapchainExtent = surfCaps.currentExtent;
	}

	// figure out the best present mode, mailo
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (i = 0; i < numPresentModes; i++)
	{
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
		{
			swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	// get the optimal set of swap chain images, the more the better
	uint32_t numSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (numSwapchainImages > surfCaps.maxImageCount)) numSwapchainImages = surfCaps.maxImageCount;

	// create a transform
	VkSurfaceTransformFlagBitsKHR transform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else																	  transform = surfCaps.currentTransform;

	VkSwapchainCreateInfoKHR swapchainInfo =
	{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		NULL,
		0,
		this->surface,
		numSwapchainImages,
		this->format,
		this->colorSpace,
		swapchainExtent,
		1,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		NULL,
		transform,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		swapchainPresentMode,
		true,
		VK_NULL_HANDLE
	};

	// create swapchain
	res = vkCreateSwapchainKHR(VkRenderDevice::dev, &swapchainInfo, NULL, &this->swapchain);
	n_assert(res == VK_SUCCESS);

	// get back buffers
	res = vkGetSwapchainImagesKHR(VkRenderDevice::dev, this->swapchain, &this->numBackbuffers, NULL);
	n_assert(res == VK_SUCCESS);

	this->backbuffers.Resize(this->numBackbuffers);
	this->backbufferSemaphores.Resize(this->numBackbuffers);
	res = vkGetSwapchainImagesKHR(VkRenderDevice::dev, this->swapchain, &this->numBackbuffers, this->backbuffers.Begin());

	this->backbufferViews.Resize(this->numBackbuffers);
	for (i = 0; i < this->numBackbuffers; i++)
	{
		// setup view
		VkImageViewCreateInfo backbufferViewInfo =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0,
			this->backbuffers[i],
			VK_IMAGE_VIEW_TYPE_2D,
			this->format,
			mapping,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};
		res = vkCreateImageView(VkRenderDevice::dev, &backbufferViewInfo, NULL, &this->backbufferViews[i]);
		n_assert(res == VK_SUCCESS);

		VkSemaphoreCreateInfo semaphoreCreateInfo =
		{
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			NULL,
			0
		};
		res = vkCreateSemaphore(VkRenderDevice::dev, &semaphoreCreateInfo, NULL, &this->backbufferSemaphores[i]);
		n_assert(res == VK_SUCCESS);
	}
	this->currentBackbuffer = 0;

	// create display semaphore
	const VkSemaphoreCreateInfo semInfo =
	{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		NULL,
		0
	};
	vkCreateSemaphore(VkRenderDevice::dev, &semInfo, NULL, &this->displaySemaphore);

	// get present queue
	uint32_t numQueues = VkRenderDevice::Instance()->numQueues;
	for (i = 0; i < numQueues; i++)
	{
		VkBool32 canPresent;
		vkGetPhysicalDeviceSurfaceSupportKHR(VkRenderDevice::physicalDev, i, this->surface, &canPresent);
		if (canPresent)
		{
			this->presentQueue = VkRenderDevice::queues[i];
			break;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::DiscardVulkanSwapchain()
{
	uint32_t i;
	for (i = 0; i < this->numBackbuffers; i++)
	{
		vkDestroyImageView(VkRenderDevice::dev, this->backbufferViews[i], NULL);
	}

	// destroy swapchain last
	vkDestroySwapchainKHR(VkRenderDevice::dev, this->swapchain, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
GLFWWindow::SubmitWindowPresent()
{
	// submit a sync point for the display, transfer bit is viable since we blit to the texture
	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
	const VkSubmitInfo submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		NULL,
		1,
		&this->displaySemaphore,
		&flags,
		0,
		NULL,
		0,
		NULL
	};
	VkResult res = vkQueueSubmit(this->presentQueue, 1, &submitInfo, NULL);
	n_assert(res == VK_SUCCESS);

	// present
	VkResult presentResults;
	const VkPresentInfoKHR info =
	{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		NULL,
		0,
		NULL,
		1,
		&this->swapchain,
		&this->currentBackbuffer,
		&presentResults
	};
	res = vkQueuePresentKHR(this->presentQueue, &info);


	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// window has been resized!
		n_printf("Window resized!");
	}
	else
	{
		n_assert(res == VK_SUCCESS);
	}
}



#endif

} // namespace GLFW