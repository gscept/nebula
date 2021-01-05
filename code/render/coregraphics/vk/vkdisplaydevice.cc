//------------------------------------------------------------------------------
// vkdisplaydevice.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkdisplaydevice.h"
#include "vkrenderdevice.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkDisplayDevice, 'VKDD', Base::DisplayDeviceBase);
__ImplementSingleton(Vulkan::VkDisplayDevice);
//------------------------------------------------------------------------------
/**
*/
VkDisplayDevice::VkDisplayDevice()
{
    __ConstructSingleton;
    glfwInit();
}

//------------------------------------------------------------------------------
/**
*/
VkDisplayDevice::~VkDisplayDevice()
{
    __DestructSingleton;
    glfwTerminate();
}

//------------------------------------------------------------------------------
/**
*/
bool
VkDisplayDevice::WindowOpen()
{
    bool res = false;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    res |= OpenGL4::GLFWDisplayDevice::WindowOpen();
    return res;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkDisplayDevice::EmbedWindow()
{
    bool res = false;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    res |= OpenGL4::GLFWDisplayDevice::EmbedWindow();
    return res;
}

//------------------------------------------------------------------------------
/**
*/
void
VkDisplayDevice::SetupSwapchain()
{
    // create surface in window
    const VkInstance& inst = VkRenderDevice::Instance()->instance;

    VkResult status;
    status = glfwCreateWindowSurface(inst, this->window, NULL, &this->surface);
    
    n_assert(status == VK_SUCCESS);
}

} // namespace Vulkan