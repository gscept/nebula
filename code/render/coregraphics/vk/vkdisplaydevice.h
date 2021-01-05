#pragma once
//------------------------------------------------------------------------------
/**
    Implements a Vulkan display device.
    
    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/glfw/glfwdisplaydevice.h"
namespace Vulkan
{
class VkDisplayDevice : public GLFW::GLFWDisplayDevice
{
    __DeclareClass(VkDisplayDevice);
    __DeclareSingleton(VkDisplayDevice);
public:
    /// constructor
    VkDisplayDevice();
    /// destructor
    virtual ~VkDisplayDevice();
private:
    friend class VkRenderDevice;

    /// open window
    bool WindowOpen();
    /// open embedded window
    bool EmbedWindow();

    /// setup swapchain
    void SetupSwapchain();

    VkSurfaceKHR surface;
};
} // namespace Vulkan