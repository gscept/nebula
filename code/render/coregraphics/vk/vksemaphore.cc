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
	VkSemaphoreCreateInfo cinfo =
	{
		VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		nullptr,
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

//------------------------------------------------------------------------------
/**
*/
void
SemaphoreWait(const SemaphoreId& semaphore)
{
}

//------------------------------------------------------------------------------
/**
*/
void
SemaphoreSignal(const SemaphoreId& semaphore)
{
}

} // namespace Vulkan

#pragma pop_macro("CreateSemaphore")