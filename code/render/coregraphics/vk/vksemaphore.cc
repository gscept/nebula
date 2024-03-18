//------------------------------------------------------------------------------
//  vksemaphore.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vksemaphore.h"
#include "vkgraphicsdevice.h"

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif

namespace Vulkan
{
VkSemaphoreAllocator semaphoreAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
VkSemaphore
SemaphoreGetVk(const CoreGraphics::SemaphoreId& id)
{
    if (id == CoreGraphics::SemaphoreId::Invalid()) return VK_NULL_HANDLE;
    else                                            return semaphoreAllocator.Get<Semaphore_VkHandle>(id.id);
}
}

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
SemaphoreId
CreateSemaphore(const SemaphoreCreateInfo& info)
{
    VkSemaphoreTypeCreateInfoKHR ext =
    {
        VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR,
        nullptr,
        info.type == Binary ? VkSemaphoreTypeKHR::VK_SEMAPHORE_TYPE_BINARY_KHR : VkSemaphoreTypeKHR::VK_SEMAPHORE_TYPE_TIMELINE_KHR,
        0
    };
    VkSemaphoreCreateInfo cinfo =
    {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        &ext,
        0
    };

    Ids::Id32 id = semaphoreAllocator.Alloc();
    VkDevice dev = Vulkan::GetCurrentDevice();
    semaphoreAllocator.Get<Semaphore_Device>(id) = dev;
    semaphoreAllocator.Get<Semaphore_Type>(id) = info.type;

    VkResult res = vkCreateSemaphore(dev, &cinfo, nullptr, &semaphoreAllocator.Get<Semaphore_VkHandle>(id));
    n_assert(res == VK_SUCCESS);

    SemaphoreId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroySemaphore(const SemaphoreId& semaphore)
{
    vkDestroySemaphore(semaphoreAllocator.Get<Semaphore_Device>(semaphore.id), semaphoreAllocator.Get<Semaphore_VkHandle>(semaphore.id), nullptr);
    semaphoreAllocator.Dealloc(semaphore.id);
}

//------------------------------------------------------------------------------
/**
*/
uint64 
SemaphoreGetValue(const SemaphoreId& semaphore)
{
    return semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id);
}

//------------------------------------------------------------------------------
/**
*/
void 
SemaphoreSignal(const SemaphoreId& semaphore)
{
    switch (semaphoreAllocator.Get<Semaphore_Type>(semaphore.id))
    {
    case SemaphoreType::Binary:
        semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id) = 1;
        break;
    case SemaphoreType::Timeline:
        semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id)++;
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
SemaphoreReset(const SemaphoreId& semaphore)
{
    switch (semaphoreAllocator.Get<Semaphore_Type>(semaphore.id))
    {
    case SemaphoreType::Binary:
        semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id) = 0;
        break;
    default: n_error("unhandled enum"); break;
    }
}

} // namespace Vulkan

#pragma pop_macro("CreateSemaphore")
