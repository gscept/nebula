#pragma once
//------------------------------------------------------------------------------
/**
    A swapchain represents a set of backbuffers which the monitor fetches it's framebuffer from

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/config.h"
#include "coregraphics/displaydevice.h"
#include "ids/idallocator.h"

namespace CoreGraphics
{

ID_24_8_TYPE(SwapchainId);

struct SwapchainCreateInfo
{
    CoreGraphics::DisplayMode displayMode;
    bool vsync;
    GLFWwindow* window;
};

/// Create swapchain
SwapchainId CreateSwapchain(const SwapchainCreateInfo& info);
/// Destroy swapchain
void DestroySwapchain(const SwapchainId id);

/// Swap buffers
void SwapchainSwap(const SwapchainId id);
/// Copy to current backbuffer (call after swap)
void SwapchainCopy(const SwapchainId id, const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::TextureId source);
/// Present
void SwapchainPresent(const SwapchainId id);

} // namespace CoreGraphics
