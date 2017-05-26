#pragma once
//------------------------------------------------------------------------------
/**
	Implements a window using GLFW
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/windowbase.h"
#include "GLFW/glfw3.h"

#if __VULKAN__
namespace Vulkan
{
	class VkRenderDevice;
	class VkRenderTexture;
}
#endif

namespace GLFW
{
class GLFWWindow : public Base::WindowBase
{
	__DeclareClass(GLFWWindow);
public:
	/// constructor
	GLFWWindow();
	/// destructor
	virtual ~GLFWWindow();

	/// open window
	void Open();
	/// close window
	void Close();
	/// reopen window (using new width and height)
	void Reopen();

	/// called when the view becomes current
	void MakeCurrent();

	/// swap buffers
	void Present(IndexT frameIndex);

	/// set window title
	void SetTitle(const Util::String& title);
	/// set windowed/fullscreen mode
	void SetFullscreen(bool b, int monitor);
	/// set cursor visible
	void SetCursorVisible(bool visible);
	/// set cursor locked to window
	void SetCursorLocked(bool locked);

	/// declare static key function as friend
	friend void staticKeyFunc(GLFWwindow *window, int key, int scancode, int action, int mods);
	/// declare static mouse button function as friend
	friend void staticMouseButtonFunc(GLFWwindow *window, int button, int action, int mods);
	/// declare static mouse function as friend
	friend void staticMouseFunc(GLFWwindow *window, double xpos, double ypos);
	/// declare static scroll function as friend
	friend void staticScrollFunc(GLFWwindow *window, double xs, double ys);
	/// declare static close function as friend
	friend void staticCloseFunc(GLFWwindow * window);
	/// declare static focus function as friend
	friend void staticFocusFunc(GLFWwindow * window, int focus);
	/// declare static size function as friend
	friend void staticSizeFunc(GLFWwindow* window, int width, int height);
	/// declare static char function as friend
	friend void staticCharFunc(GLFWwindow* window, unsigned int key);
protected:
#ifdef __OGL4__
	friend class OpenGL4::OGL4RenderDevice;
#elseif __VULKAN__
    friend class Vulkan::VkRenderDevice;
#endif

	/// Keyboard callback
	void KeyFunc(int key, int scancode, int action, int mods);
	/// Character callback
	void CharFunc(unsigned int key);
	/// Mouse Button callback
	void MouseButtonFunc(int button, int action, int mods);
	/// Mouse callback
	void MouseFunc(double xpos, double ypos);
	/// Scroll callback
	void ScrollFunc(double xs, double ys);
	/// window close func
	void CloseFunc();
	/// window focus
	void FocusFunc(int focus);
	/// window resize
	void ResizeFunc(int width, int height);

	/// enables callbacks
	void EnableCallbacks();
	/// disables callbacks
	void DisableCallbacks();

	/// apply fullscreen
	void ApplyFullscreen();

	// not a pointer to self!
	GLFWwindow* window;

#if __VULKAN__
	friend class Vulkan::VkRenderTexture;
	friend class Vulkan::VkRenderDevice;
	VkSurfaceKHR surface;

	/// get surface
	const VkSurfaceKHR& GetSurface() const;
	/// setup Vulkan swapchain
	void SetupVulkanSwapchain();
	/// destroy Vulkan swapchain
	void DiscardVulkanSwapchain();

	/// submit window present to queue
	void SubmitWindowPresent();

	VkFormat format;
	VkColorSpaceKHR colorSpace;
	VkSwapchainKHR swapchain;
	VkSemaphore displaySemaphore;
	VkQueue presentQueue;

	uint32_t currentBackbuffer;
	Util::FixedArray<VkImage> backbuffers;
	Util::FixedArray<VkImageView> backbufferViews;
	Util::FixedArray<VkSemaphore> backbufferSemaphores;
	uint32_t numBackbuffers;
#endif
};

//------------------------------------------------------------------------------
/**
*/
inline void
GLFWWindow::Present(IndexT frameIndex)
{
	n_assert(this->window != 0);
	if (this->swapFrame != frameIndex)
	{
#if __OGL4__
		glfwSwapBuffers(this->window);
#elif __VULKAN__
		this->SubmitWindowPresent();
#endif
		this->swapFrame = frameIndex;
	}
}

#if __VULKAN__
//------------------------------------------------------------------------------
/**
*/
inline const VkSurfaceKHR&
GLFWWindow::GetSurface() const
{
	return this->surface;
}
#endif

} // namespace GLFW