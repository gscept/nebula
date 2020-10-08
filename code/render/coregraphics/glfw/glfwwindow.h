#pragma once
//------------------------------------------------------------------------------
/**
	Implements a window using GLFW
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "GLFW/glfw3.h"
#include "util/fixedarray.h"
#include "coregraphics/window.h"
#include "coregraphics/displaymode.h"
#include "ids/idallocator.h"

#if __VULKAN__

namespace Vulkan
{

struct VkWindowSwapInfo
{
	VkDevice dev;
	VkSwapchainKHR swapchain;
	VkQueue presentQueue;

	uint32_t currentBackbuffer;
};

struct VkSwapchainInfo
{
	VkFormat format;
	VkColorSpaceKHR colorSpace;
	VkSurfaceKHR surface;
};

struct VkBackbufferInfo
{
	Util::FixedArray<VkImage> backbuffers;
	Util::FixedArray<VkImageView> backbufferViews;
	uint32_t numBackbuffers;
};

/// get surface
const VkSurfaceKHR& GetSurface(const CoreGraphics::WindowId& id);
/// setup Vulkan swapchain
void SetupVulkanSwapchain(const CoreGraphics::WindowId& id, const CoreGraphics::DisplayMode& mode, const Util::StringAtom& title);
/// destroy Vulkan swapchain
void DiscardVulkanSwapchain(const CoreGraphics::WindowId& id);
/// recreate Vulkan swapchain (used when swapchain is no longer valid due to resize of window or similar)
void RecreateVulkanSwapchain(const CoreGraphics::WindowId& id, const CoreGraphics::DisplayMode& mode, const Util::StringAtom& title);
/// perform a present
void Present(const CoreGraphics::WindowId& id);

} // namespace Vulkan
#endif

namespace CoreGraphics
{

enum
{
	GLFW_Window,
	GLFW_DisplayMode,
	GLFW_SwapFrame,
	GLFW_SetupInfo,
	GLFW_Texture,
#if __VULKAN__
	GLFW_WindowSwapInfo,
	GLFW_SwapChain,
	GLFW_Backbuffer
#endif
};


typedef Ids::IdAllocator<
	  GLFWwindow*					
	, CoreGraphics::DisplayMode		
	, IndexT						
	, WindowCreateInfo
	, TextureId
#if __VULKAN__
	, Vulkan::VkWindowSwapInfo		
	, Vulkan::VkSwapchainInfo		
	, Vulkan::VkBackbufferInfo		
#endif
> GLFWWindowAllocatorType;
extern GLFWWindowAllocatorType glfwWindowAllocator;
} // namespace CoreGraphics

namespace GLFW
{

/// Keyboard callback
void KeyFunc(const CoreGraphics::WindowId& id, int key, int scancode, int action, int mods);
/// Character callback
void CharFunc(const CoreGraphics::WindowId& id, unsigned int key);
/// Mouse Button callback
void MouseButtonFunc(const CoreGraphics::WindowId& id, int button, int action, int mods);
/// Mouse callback
void MouseFunc(const CoreGraphics::WindowId& id, double xpos, double ypos);
/// Scroll callback
void ScrollFunc(const CoreGraphics::WindowId& id, double xs, double ys);
/// window close func
void CloseFunc(const CoreGraphics::WindowId& id);
/// window focus
void FocusFunc(const CoreGraphics::WindowId& id, int focus);
/// window resize
void ResizeFunc(const CoreGraphics::WindowId& id, int width, int height);

/// declare static key function as friend
void staticKeyFunc(GLFWwindow *window, int key, int scancode, int action, int mods);
/// declare static mouse button function as friend
void staticMouseButtonFunc(GLFWwindow *window, int button, int action, int mods);
/// declare static mouse function as friend
void staticMouseFunc(GLFWwindow *window, double xpos, double ypos);
/// declare static scroll function as friend
void staticScrollFunc(GLFWwindow *window, double xs, double ys);
/// declare static close function as friend
void staticCloseFunc(GLFWwindow * window);
/// declare static focus function as friend
void staticFocusFunc(GLFWwindow * window, int focus);
/// declare static size function as friend
void staticSizeFunc(GLFWwindow* window, int width, int height);
/// declare static char function as friend
void staticCharFunc(GLFWwindow* window, unsigned int key);

/// enables callbacks
void EnableCallbacks(const CoreGraphics::WindowId& id);
/// disables callbacks
void DisableCallbacks(const CoreGraphics::WindowId& id);

/// internal setup function, either does embedding or ordinary opening
const CoreGraphics::WindowId InternalSetupFunction(const CoreGraphics::WindowCreateInfo& info, const Util::Blob& windowData, bool embed);

}
