//------------------------------------------------------------------------------
//  vkloader.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkloader.h"
#include "core/debug.h"
#ifdef __linux__
#include <dlfcn.h>
#endif
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
void InitVulkan()
{
#if __WIN32__
    HMODULE vulkanLib = LoadLibraryA("vulkan-1.dll");
    if (!vulkanLib)
        n_error("Could not find 'vulkan-1.dll', make sure you have installed vulkan");

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkanLib, "vkGetInstanceProcAddr");
#elif __linux__
    void* vulkanLib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!vulkanLib) vulkanLib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if(!vulkanLib)
    {
        n_error("Could not find 'libvulkan', make sure you have installed vulkan");
    }

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vulkanLib, "vkGetInstanceProcAddr");
#elif ( __OSX__ || __APPLE__ )
    void* vulkanLib = dlopen("libvulkan.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vulkanLib)
        vulkanLib = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!vulkanLib)
        vulkanLib = dlopen("libMoltenVK.dylib", RTLD_NOW | RTLD_LOCAL);
    else
        n_error("Could not find 'libvulkan', make sure you have installed vulkan");

    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(vulkanLib, "vkGetInstanceProcAddr");
#else
#error "Vulkan not supported on your platform!"
#endif

    // setup non-instance calls
    VkInstance instance = VK_NULL_HANDLE;
    _IMP_VK(vkCreateInstance);
    _IMP_VK(vkEnumerateInstanceExtensionProperties);
    _IMP_VK(vkEnumerateInstanceLayerProperties);
    _IMP_VK(vkEnumerateInstanceVersion);
}

//------------------------------------------------------------------------------
/**
*/
void 
InitInstance(VkInstance instance)
{
    // implement instance functions
    _IMP_VK(vkCreateDevice);
    _IMP_VK(vkDestroyDevice);
    _IMP_VK(vkDestroyInstance);
    _IMP_VK(vkDeviceWaitIdle);
    _IMP_VK(vkGetDeviceQueue);

    _IMP_VK(vkGetPhysicalDeviceSurfaceFormatsKHR);
    _IMP_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    _IMP_VK(vkGetPhysicalDeviceSurfacePresentModesKHR);
    _IMP_VK(vkGetPhysicalDeviceSurfaceSupportKHR);
    _IMP_VK(vkCreateSwapchainKHR);
    _IMP_VK(vkGetSwapchainImagesKHR);
    _IMP_VK(vkAcquireNextImageKHR);
    _IMP_VK(vkDestroySwapchainKHR);
    _IMP_VK(vkQueuePresentKHR);

    _IMP_VK(vkQueueSubmit);
    _IMP_VK(vkQueueBindSparse);
    _IMP_VK(vkQueueWaitIdle);

    _IMP_VK(vkEnumerateDeviceExtensionProperties);
    _IMP_VK(vkEnumerateDeviceLayerProperties);
    _IMP_VK(vkEnumeratePhysicalDevices);
    _IMP_VK(vkGetDeviceProcAddr);

    _IMP_VK(vkCreatePipelineCache);
    _IMP_VK(vkDestroyPipelineCache);
    _IMP_VK(vkGetPipelineCacheData);
    _IMP_VK(vkCreateQueryPool);
    _IMP_VK(vkDestroyQueryPool);
    _IMP_VK(vkResetQueryPool);

    // physical device
    _IMP_VK(vkGetPhysicalDeviceProperties);
    _IMP_VK(vkGetPhysicalDeviceFeatures);
    _IMP_VK(vkGetPhysicalDeviceQueueFamilyProperties);
    _IMP_VK(vkGetPhysicalDeviceMemoryProperties);
    _IMP_VK(vkGetPhysicalDeviceFormatProperties);
    _IMP_VK(vkGetPhysicalDeviceSparseImageFormatProperties);
    _IMP_VK(vkGetImageSparseMemoryRequirements);

    // command buffer
    _IMP_VK(vkCmdDraw);
    _IMP_VK(vkCmdDrawIndexed);
    _IMP_VK(vkCmdDrawIndirect);
    _IMP_VK(vkCmdDrawIndexedIndirect);
    _IMP_VK(vkCmdDispatch);
    _IMP_VK(vkCmdResolveImage);

    _IMP_VK(vkCmdCopyImage);
    _IMP_VK(vkCmdBlitImage);
    _IMP_VK(vkCmdCopyBuffer);
    _IMP_VK(vkCmdUpdateBuffer);
    _IMP_VK(vkCmdCopyBufferToImage);
    _IMP_VK(vkCmdCopyImageToBuffer);

    _IMP_VK(vkCmdBindDescriptorSets);
    _IMP_VK(vkCmdPushConstants);
    _IMP_VK(vkCmdSetViewport);
    _IMP_VK(vkCmdSetScissor);
    _IMP_VK(vkCmdSetStencilCompareMask);
    _IMP_VK(vkCmdSetStencilWriteMask);
    _IMP_VK(vkCmdSetStencilReference);
    _IMP_VK(vkCmdSetPrimitiveTopology);
    _IMP_VK(vkCmdSetPrimitiveRestartEnable);
    _IMP_VK(vkCmdSetVertexInputEXT);

    _IMP_VK(vkCreateCommandPool);
    _IMP_VK(vkDestroyCommandPool);
    _IMP_VK(vkAllocateCommandBuffers);
    _IMP_VK(vkFreeCommandBuffers);
    _IMP_VK(vkCmdExecuteCommands);
    _IMP_VK(vkBeginCommandBuffer);
    _IMP_VK(vkEndCommandBuffer);
    _IMP_VK(vkResetCommandBuffer);

    _IMP_VK(vkCreateEvent);
    _IMP_VK(vkDestroyEvent);
    _IMP_VK(vkCmdSetEvent);
    _IMP_VK(vkCmdWaitEvents);
    _IMP_VK(vkCmdResetEvent);
    _IMP_VK(vkGetEventStatus);
    _IMP_VK(vkResetEvent);
    _IMP_VK(vkSetEvent);

    _IMP_VK(vkCmdPipelineBarrier);
    _IMP_VK(vkCreateFence);
    _IMP_VK(vkDestroyFence);
    _IMP_VK(vkResetFences);
    _IMP_VK(vkGetFenceStatus);
    _IMP_VK(vkWaitForFences);
    _IMP_VK(vkWaitSemaphores);
    _IMP_VK(vkGetSemaphoreCounterValue);

    _IMP_VK(vkCreateRenderPass);
    _IMP_VK(vkDestroyRenderPass);
    _IMP_VK(vkCmdBeginRenderPass);
    _IMP_VK(vkCmdEndRenderPass);
    _IMP_VK(vkCmdNextSubpass);
    _IMP_VK(vkCmdBindIndexBuffer);
    _IMP_VK(vkCmdBindVertexBuffers);
    _IMP_VK(vkCmdClearDepthStencilImage);
    _IMP_VK(vkCmdClearColorImage);
    _IMP_VK(vkCreateFramebuffer);
    _IMP_VK(vkDestroyFramebuffer);

    _IMP_VK(vkCreateGraphicsPipelines);
    _IMP_VK(vkCreateComputePipelines);
    _IMP_VK(vkDestroyPipeline);
    _IMP_VK(vkCreateShaderModule);
    _IMP_VK(vkDestroyShaderModule);
    _IMP_VK(vkCreateDescriptorSetLayout);
    _IMP_VK(vkDestroyDescriptorSetLayout);
    _IMP_VK(vkCreateDescriptorPool);
    _IMP_VK(vkDestroyDescriptorPool);
    _IMP_VK(vkAllocateDescriptorSets);
    _IMP_VK(vkFreeDescriptorSets);
    _IMP_VK(vkUpdateDescriptorSets);
    _IMP_VK(vkCreatePipelineLayout);
    _IMP_VK(vkDestroyPipelineLayout);
    _IMP_VK(vkCreateSampler);
    _IMP_VK(vkDestroySampler);

    _IMP_VK(vkCmdBindPipeline);
    _IMP_VK(vkCmdCopyQueryPoolResults);
    _IMP_VK(vkCmdResetQueryPool);
    _IMP_VK(vkCmdWriteTimestamp);
    _IMP_VK(vkCmdBeginQuery);
    _IMP_VK(vkCmdEndQuery);

    _IMP_VK(vkCreateSemaphore);
    _IMP_VK(vkDestroySemaphore);

    // memory
    _IMP_VK(vkAllocateMemory);
    _IMP_VK(vkFreeMemory);
    _IMP_VK(vkMapMemory);
    _IMP_VK(vkUnmapMemory);
    _IMP_VK(vkFlushMappedMemoryRanges);
    _IMP_VK(vkInvalidateMappedMemoryRanges);

    _IMP_VK(vkGetBufferMemoryRequirements);
    _IMP_VK(vkGetImageMemoryRequirements);

    _IMP_VK(vkCreateBuffer);
    _IMP_VK(vkCreateBufferView);
    _IMP_VK(vkDestroyBuffer);
    _IMP_VK(vkBindBufferMemory);

    _IMP_VK(vkCreateImage);
    _IMP_VK(vkDestroyImage);
    _IMP_VK(vkBindImageMemory);
    _IMP_VK(vkCreateImageView);
    _IMP_VK(vkDestroyImageView);

    _IMP_VK(vkGetAccelerationStructureBuildSizesKHR);
    _IMP_VK(vkCreateAccelerationStructureKHR);
    _IMP_VK(vkBuildAccelerationStructuresKHR);
    _IMP_VK(vkGetAccelerationStructureDeviceAddressKHR);
    _IMP_VK(vkGetDeviceAccelerationStructureCompatibilityKHR);
    _IMP_VK(vkGetBufferDeviceAddressKHR);
}

} // namespace Vulkan


// declare context functions
_DEF_VK(vkGetInstanceProcAddr);
_DEF_VK(vkCreateInstance);
_DEF_VK(vkEnumerateInstanceExtensionProperties);
_DEF_VK(vkEnumerateInstanceLayerProperties);
_DEF_VK(vkEnumerateInstanceVersion);

// DEFlare instance functions
_DEF_VK(vkCreateDevice);
_DEF_VK(vkDestroyDevice);
_DEF_VK(vkDestroyInstance);
_DEF_VK(vkDeviceWaitIdle);
_DEF_VK(vkGetDeviceQueue);

_DEF_VK(vkGetPhysicalDeviceSurfaceFormatsKHR);
_DEF_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
_DEF_VK(vkGetPhysicalDeviceSurfacePresentModesKHR);
_DEF_VK(vkGetPhysicalDeviceSurfaceSupportKHR);
_DEF_VK(vkCreateSwapchainKHR);
_DEF_VK(vkGetSwapchainImagesKHR);
_DEF_VK(vkAcquireNextImageKHR);
_DEF_VK(vkDestroySwapchainKHR);
_DEF_VK(vkQueuePresentKHR);

_DEF_VK(vkQueueSubmit);
_DEF_VK(vkQueueBindSparse);
_DEF_VK(vkQueueWaitIdle);

_DEF_VK(vkEnumerateDeviceExtensionProperties);
_DEF_VK(vkEnumerateDeviceLayerProperties);
_DEF_VK(vkEnumeratePhysicalDevices);
_DEF_VK(vkGetDeviceProcAddr);

_DEF_VK(vkCreatePipelineCache);
_DEF_VK(vkDestroyPipelineCache);
_DEF_VK(vkGetPipelineCacheData);
_DEF_VK(vkCreateQueryPool);
_DEF_VK(vkDestroyQueryPool);
_DEF_VK(vkResetQueryPool);

// physical device
_DEF_VK(vkGetPhysicalDeviceProperties);
_DEF_VK(vkGetPhysicalDeviceFeatures);
_DEF_VK(vkGetPhysicalDeviceQueueFamilyProperties);
_DEF_VK(vkGetPhysicalDeviceMemoryProperties);
_DEF_VK(vkGetPhysicalDeviceFormatProperties);
_DEF_VK(vkGetPhysicalDeviceSparseImageFormatProperties);
_DEF_VK(vkGetImageSparseMemoryRequirements);

// command buffer
_DEF_VK(vkCmdDraw);
_DEF_VK(vkCmdDrawIndexed);
_DEF_VK(vkCmdDrawIndirect);
_DEF_VK(vkCmdDrawIndexedIndirect);
_DEF_VK(vkCmdDispatch);
_DEF_VK(vkCmdResolveImage);

_DEF_VK(vkCmdCopyImage);
_DEF_VK(vkCmdBlitImage);
_DEF_VK(vkCmdCopyBuffer);
_DEF_VK(vkCmdUpdateBuffer);
_DEF_VK(vkCmdCopyBufferToImage);
_DEF_VK(vkCmdCopyImageToBuffer);

_DEF_VK(vkCmdBindDescriptorSets);
_DEF_VK(vkCmdPushConstants);
_DEF_VK(vkCmdSetViewport);
_DEF_VK(vkCmdSetScissor);
_DEF_VK(vkCmdSetStencilCompareMask);
_DEF_VK(vkCmdSetStencilWriteMask);
_DEF_VK(vkCmdSetStencilReference);
_DEF_VK(vkCmdSetPrimitiveTopology);
_DEF_VK(vkCmdSetPrimitiveRestartEnable);
_DEF_VK(vkCmdSetVertexInputEXT);

_DEF_VK(vkCreateCommandPool);
_DEF_VK(vkDestroyCommandPool);
_DEF_VK(vkAllocateCommandBuffers);
_DEF_VK(vkFreeCommandBuffers);
_DEF_VK(vkCmdExecuteCommands);
_DEF_VK(vkBeginCommandBuffer);
_DEF_VK(vkEndCommandBuffer);
_DEF_VK(vkResetCommandBuffer);

_DEF_VK(vkCreateEvent);
_DEF_VK(vkDestroyEvent);
_DEF_VK(vkCmdSetEvent);
_DEF_VK(vkCmdWaitEvents);
_DEF_VK(vkCmdResetEvent);
_DEF_VK(vkGetEventStatus);
_DEF_VK(vkResetEvent);
_DEF_VK(vkSetEvent);

_DEF_VK(vkCmdPipelineBarrier);
_DEF_VK(vkCreateFence);
_DEF_VK(vkDestroyFence);
_DEF_VK(vkResetFences);
_DEF_VK(vkGetFenceStatus);
_DEF_VK(vkWaitForFences);
_DEF_VK(vkWaitSemaphores);
_DEF_VK(vkGetSemaphoreCounterValue);

_DEF_VK(vkCreateRenderPass);
_DEF_VK(vkDestroyRenderPass);
_DEF_VK(vkCmdBeginRenderPass);
_DEF_VK(vkCmdEndRenderPass);
_DEF_VK(vkCmdNextSubpass);
_DEF_VK(vkCmdBindIndexBuffer);
_DEF_VK(vkCmdBindVertexBuffers);
_DEF_VK(vkCmdClearDepthStencilImage);
_DEF_VK(vkCmdClearColorImage);
_DEF_VK(vkCreateFramebuffer);
_DEF_VK(vkDestroyFramebuffer);

_DEF_VK(vkCreateGraphicsPipelines);
_DEF_VK(vkCreateComputePipelines);
_DEF_VK(vkDestroyPipeline);
_DEF_VK(vkCreateShaderModule);
_DEF_VK(vkDestroyShaderModule);
_DEF_VK(vkCreateDescriptorSetLayout);
_DEF_VK(vkDestroyDescriptorSetLayout);
_DEF_VK(vkCreateDescriptorPool);
_DEF_VK(vkDestroyDescriptorPool);
_DEF_VK(vkAllocateDescriptorSets);
_DEF_VK(vkFreeDescriptorSets);
_DEF_VK(vkUpdateDescriptorSets);
_DEF_VK(vkCreatePipelineLayout);
_DEF_VK(vkDestroyPipelineLayout);
_DEF_VK(vkCreateSampler);
_DEF_VK(vkDestroySampler);

_DEF_VK(vkCmdBindPipeline);
_DEF_VK(vkCmdCopyQueryPoolResults);
_DEF_VK(vkCmdResetQueryPool);
_DEF_VK(vkCmdWriteTimestamp);
_DEF_VK(vkCmdBeginQuery);
_DEF_VK(vkCmdEndQuery);

_DEF_VK(vkCreateSemaphore);
_DEF_VK(vkDestroySemaphore);

// memory
_DEF_VK(vkAllocateMemory);
_DEF_VK(vkFreeMemory);
_DEF_VK(vkMapMemory);
_DEF_VK(vkUnmapMemory);
_DEF_VK(vkFlushMappedMemoryRanges);
_DEF_VK(vkInvalidateMappedMemoryRanges);

_DEF_VK(vkGetBufferMemoryRequirements);
_DEF_VK(vkGetImageMemoryRequirements);

_DEF_VK(vkCreateBuffer);
_DEF_VK(vkCreateBufferView);
_DEF_VK(vkDestroyBuffer);
_DEF_VK(vkBindBufferMemory);

_DEF_VK(vkCreateImage);
_DEF_VK(vkDestroyImage);
_DEF_VK(vkBindImageMemory);
_DEF_VK(vkCreateImageView);
_DEF_VK(vkDestroyImageView);

_DEF_VK(vkGetAccelerationStructureBuildSizesKHR);
_DEF_VK(vkCreateAccelerationStructureKHR);
_DEF_VK(vkBuildAccelerationStructuresKHR);
_DEF_VK(vkGetAccelerationStructureDeviceAddressKHR);
_DEF_VK(vkGetDeviceAccelerationStructureCompatibilityKHR);
_DEF_VK(vkGetBufferDeviceAddressKHR);
