#pragma once
//------------------------------------------------------------------------------
/**
    Vulkan layer and functions loader, use to bypass having to link to
    vulkan-1.lib and load the functions directly, gaining some performance.

    Add a function with _DEC_VK, implement said function in InitInstance using
    _IMP_VK, and use _DEF_VK outside the namespace in the cc file to define
    the function.

    Currently only supports a single VkInstance.

    Inspired by volk. 

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
namespace Vulkan
{

/// initialize Vulkan by loading dll and setting up the instance loader
extern void InitVulkan();
/// initialize Vulkan instance, loads function pointers directly from driver
extern void InitInstance(VkInstance instance);

} // namespace Vulkan

#define _IMP_VK(name) name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);n_assert_fmt(name != nullptr, "Unable to get function proc: %s\n",#name);
#define _DEC_VK(name) extern PFN_##name name;
#define _DEF_VK(name) PFN_##name name;

#ifdef __cplusplus
extern "C" {
#endif

// declare context functions
_DEC_VK(vkGetInstanceProcAddr);
_DEC_VK(vkCreateInstance);
_DEC_VK(vkEnumerateInstanceExtensionProperties);
_DEC_VK(vkEnumerateInstanceLayerProperties);
_DEC_VK(vkEnumerateInstanceVersion);

// declare instance functions
_DEC_VK(vkCreateDevice);
_DEC_VK(vkDestroyDevice);
_DEC_VK(vkDestroyInstance);
_DEC_VK(vkDeviceWaitIdle);
_DEC_VK(vkGetDeviceQueue);

_DEC_VK(vkGetPhysicalDeviceSurfaceFormatsKHR);
_DEC_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
_DEC_VK(vkGetPhysicalDeviceSurfacePresentModesKHR);
_DEC_VK(vkGetPhysicalDeviceSurfaceSupportKHR);
_DEC_VK(vkCreateSwapchainKHR);
_DEC_VK(vkGetSwapchainImagesKHR);
_DEC_VK(vkAcquireNextImageKHR);
_DEC_VK(vkDestroySwapchainKHR);
_DEC_VK(vkQueuePresentKHR);

_DEC_VK(vkQueueSubmit);
_DEC_VK(vkQueueBindSparse);
_DEC_VK(vkQueueWaitIdle);

_DEC_VK(vkEnumerateDeviceExtensionProperties);
_DEC_VK(vkEnumerateDeviceLayerProperties);
_DEC_VK(vkEnumeratePhysicalDevices);
_DEC_VK(vkGetDeviceProcAddr);

_DEC_VK(vkCreatePipelineCache);
_DEC_VK(vkDestroyPipelineCache);
_DEC_VK(vkGetPipelineCacheData);
_DEC_VK(vkCreateQueryPool);
_DEC_VK(vkDestroyQueryPool);
_DEC_VK(vkResetQueryPool);

// physical device
_DEC_VK(vkGetPhysicalDeviceProperties);
_DEC_VK(vkGetPhysicalDeviceFeatures);
_DEC_VK(vkGetPhysicalDeviceQueueFamilyProperties);
_DEC_VK(vkGetPhysicalDeviceMemoryProperties);
_DEC_VK(vkGetPhysicalDeviceFormatProperties);
_DEC_VK(vkGetPhysicalDeviceSparseImageFormatProperties);
_DEC_VK(vkGetImageSparseMemoryRequirements);

// command buffer
_DEC_VK(vkCmdDraw);
_DEC_VK(vkCmdDrawIndexed);
_DEC_VK(vkCmdDispatch);

_DEC_VK(vkCmdCopyImage);
_DEC_VK(vkCmdBlitImage);
_DEC_VK(vkCmdCopyBuffer);
_DEC_VK(vkCmdUpdateBuffer);
_DEC_VK(vkCmdCopyBufferToImage);
_DEC_VK(vkCmdCopyImageToBuffer);

_DEC_VK(vkCmdBindDescriptorSets);
_DEC_VK(vkCmdPushConstants);
_DEC_VK(vkCmdSetViewport);
_DEC_VK(vkCmdSetScissor);
_DEC_VK(vkCmdSetStencilCompareMask);
_DEC_VK(vkCmdSetStencilWriteMask);
_DEC_VK(vkCmdSetStencilReference);

_DEC_VK(vkCreateCommandPool);
_DEC_VK(vkDestroyCommandPool);
_DEC_VK(vkAllocateCommandBuffers);
_DEC_VK(vkFreeCommandBuffers);
_DEC_VK(vkCmdExecuteCommands);
_DEC_VK(vkBeginCommandBuffer);
_DEC_VK(vkEndCommandBuffer);
_DEC_VK(vkResetCommandBuffer);

_DEC_VK(vkCreateEvent);
_DEC_VK(vkDestroyEvent);
_DEC_VK(vkCmdSetEvent);
_DEC_VK(vkCmdWaitEvents);
_DEC_VK(vkCmdResetEvent);
_DEC_VK(vkGetEventStatus);
_DEC_VK(vkResetEvent);
_DEC_VK(vkSetEvent);

_DEC_VK(vkCmdPipelineBarrier);
_DEC_VK(vkCreateFence);
_DEC_VK(vkDestroyFence);
_DEC_VK(vkResetFences);
_DEC_VK(vkGetFenceStatus);
_DEC_VK(vkWaitForFences);
_DEC_VK(vkWaitSemaphores);

_DEC_VK(vkCreateRenderPass);
_DEC_VK(vkDestroyRenderPass);
_DEC_VK(vkCmdBeginRenderPass);
_DEC_VK(vkCmdEndRenderPass);
_DEC_VK(vkCmdNextSubpass);
_DEC_VK(vkCmdBindIndexBuffer);
_DEC_VK(vkCmdBindVertexBuffers);
_DEC_VK(vkCmdClearDepthStencilImage);
_DEC_VK(vkCmdClearColorImage);
_DEC_VK(vkCreateFramebuffer);
_DEC_VK(vkDestroyFramebuffer);

_DEC_VK(vkCreateGraphicsPipelines);
_DEC_VK(vkCreateComputePipelines);
_DEC_VK(vkDestroyPipeline);
_DEC_VK(vkCreateShaderModule);
_DEC_VK(vkDestroyShaderModule);
_DEC_VK(vkCreateDescriptorSetLayout);
_DEC_VK(vkDestroyDescriptorSetLayout);
_DEC_VK(vkCreateDescriptorPool);
_DEC_VK(vkDestroyDescriptorPool);
_DEC_VK(vkAllocateDescriptorSets);
_DEC_VK(vkFreeDescriptorSets);
_DEC_VK(vkUpdateDescriptorSets);
_DEC_VK(vkCreatePipelineLayout);
_DEC_VK(vkDestroyPipelineLayout);
_DEC_VK(vkCreateSampler);
_DEC_VK(vkDestroySampler);

_DEC_VK(vkCmdBindPipeline);
_DEC_VK(vkCmdCopyQueryPoolResults);
_DEC_VK(vkCmdResetQueryPool);
_DEC_VK(vkCmdWriteTimestamp);
_DEC_VK(vkCmdBeginQuery);
_DEC_VK(vkCmdEndQuery);

_DEC_VK(vkCreateSemaphore);
_DEC_VK(vkDestroySemaphore);

// memory

_DEC_VK(vkAllocateMemory);
_DEC_VK(vkFreeMemory);
_DEC_VK(vkMapMemory);
_DEC_VK(vkUnmapMemory);
_DEC_VK(vkFlushMappedMemoryRanges);
_DEC_VK(vkInvalidateMappedMemoryRanges);

_DEC_VK(vkGetBufferMemoryRequirements);
_DEC_VK(vkGetImageMemoryRequirements);

_DEC_VK(vkCreateBuffer);
_DEC_VK(vkCreateBufferView);
_DEC_VK(vkDestroyBuffer);
_DEC_VK(vkBindBufferMemory);

_DEC_VK(vkCreateImage);
_DEC_VK(vkDestroyImage);
_DEC_VK(vkBindImageMemory);
_DEC_VK(vkCreateImageView);
_DEC_VK(vkDestroyImageView);

#ifdef __cplusplus
}
#endif