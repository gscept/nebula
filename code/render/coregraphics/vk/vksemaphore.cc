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
	else											return semaphoreAllocator.Get<Semaphore_VkHandle>(id.id24);
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

	vkCreateSemaphore(dev, &cinfo, nullptr, &semaphoreAllocator.Get<Semaphore_VkHandle>(id));

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
	vkDestroySemaphore(semaphoreAllocator.Get<Semaphore_Device>(semaphore.id24), semaphoreAllocator.Get<Semaphore_VkHandle>(semaphore.id24), nullptr);
	semaphoreAllocator.Dealloc(semaphore.id24);
}

//------------------------------------------------------------------------------
/**
*/
uint64 
SemaphoreGetValue(const SemaphoreId& semaphore)
{
	return semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
SemaphoreSignal(const SemaphoreId& semaphore)
{
	switch (semaphoreAllocator.Get<Semaphore_Type>(semaphore.id24))
	{
	case SemaphoreType::Binary:
		semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id24) = 1;
		break;
	case SemaphoreType::Timeline:
		semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id24)++;
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
SemaphoreReset(const SemaphoreId& semaphore)
{
	switch (semaphoreAllocator.Get<Semaphore_Type>(semaphore.id24))
	{
	case SemaphoreType::Binary:
		semaphoreAllocator.Get<Semaphore_LastIndex>(semaphore.id24) = 0;
		break;
	}
}

} // namespace Vulkan

#pragma pop_macro("CreateSemaphore")