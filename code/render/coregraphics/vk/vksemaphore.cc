//------------------------------------------------------------------------------
//  vksemaphore.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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
	else											return semaphoreAllocator.Get<1>(id.id24);
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
		VkSemaphoreTypeKHR::VK_SEMAPHORE_TYPE_BINARY_KHR,
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
	semaphoreAllocator.Get<0>(id) = dev;

	vkCreateSemaphore(dev, &cinfo, nullptr, &semaphoreAllocator.Get<1>(id));

	SemaphoreId ret;
	ret.id24 = id;
	ret.id8 = SemaphoreIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroySemaphore(const SemaphoreId& semaphore)
{
	vkDestroySemaphore(semaphoreAllocator.Get<0>(semaphore.id24), semaphoreAllocator.Get<1>(semaphore.id24), nullptr);
	semaphoreAllocator.Dealloc(semaphore.id24);
}


} // namespace Vulkan

#pragma pop_macro("CreateSemaphore")