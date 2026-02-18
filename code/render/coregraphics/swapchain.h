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
#include "coregraphics/commandbuffer.h"
#include "coregraphics/texture.h"
#include "ids/idallocator.h"

namespace CoreGraphics
{

ID_24_8_TYPE(SwapchainId);

struct SwapInfo
{
    void (*syncFunc)(CoreGraphics::CmdBufferId) = nullptr;
    Math::rectangle<int> viewport;
    CoreGraphics::TextureId swapSource;
    CoreGraphics::SubmissionWaitEvent submission;
};

struct SwapchainCreateInfo
{
    CoreGraphics::DisplayMode displayMode;
    bool vsync;
    GLFWwindow* window;
    CoreGraphics::QueueType preferredQueue = CoreGraphics::GraphicsQueueType;
};

/// Create swapchain
SwapchainId CreateSwapchain(const SwapchainCreateInfo& info);
/// Destroy swapchain
void DestroySwapchain(const SwapchainId id);

/// Swap buffers
void SwapchainSwap(const SwapchainId id);
/// Get queue to use for swapchain
CoreGraphics::QueueType SwapchainGetQueueType(const SwapchainId id);
/// Allocate command buffer for swapchain operations
CoreGraphics::CmdBufferId SwapchainAllocateCmds(const SwapchainId id);
/// Copy to current backbuffer (call after swap)
void SwapchainCopy(const SwapchainId id, const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::TextureId source);
/// Present
void SwapchainPresent(const SwapchainId id);
/// Get present semaphore for the current backbuffer
CoreGraphics::SemaphoreId SwapchainGetCurrentDisplaySemaphore(const SwapchainId id);
/// Get the present fence for the current backbuffer
CoreGraphics::SemaphoreId SwapchainGetCurrentPresentSemaphore(const SwapchainId id);

/// Set window swap info
void SwapchainSetSwapInfo(const SwapchainId id, const SwapInfo& swap);
/// Get swap info for window
const SwapInfo& SwapchainGetSwapInfo(const SwapchainId id);

} // namespace CoreGraphics
