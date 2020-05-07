//------------------------------------------------------------------------------
//  vkevent.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkfence.h"
#include "coregraphics/fence.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/config.h"
#include "vktypes.h"

namespace Vulkan
{
	VkFenceAllocator fenceAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
VkFence 
FenceGetVk(const CoreGraphics::FenceId id)
{
	if (id == CoreGraphics::FenceId::Invalid()) return VK_NULL_HANDLE;
	else										return fenceAllocator.Get<1>(id.id24).fence;
}

}

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
FenceId
CreateFence(const FenceCreateInfo& info)
{
	Ids::Id32 id = fenceAllocator.Alloc();
	VkFenceCreateInfo inf =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		info.createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0
	};

	// create fence
	VkDevice dev = Vulkan::GetCurrentDevice();
	VkFence fence;
	VkResult res = vkCreateFence(dev, &inf, nullptr, &fence);
	n_assert(res == VK_SUCCESS);

	fenceAllocator.Get<0>(id) = dev;
	fenceAllocator.Get<1>(id).fence = fence;
	
	FenceId ret;
	ret.id24 = id;
	ret.id8 = FenceIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyFence(const FenceId id)
{
	VkFenceInfo& vkInfo = fenceAllocator.Get<1>(id.id24);
	const VkDevice& dev = fenceAllocator.Get<0>(id.id24);
	vkDestroyFence(dev, vkInfo.fence, nullptr);
	fenceAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
bool 
FencePeek(const FenceId id)
{
	return vkGetFenceStatus(fenceAllocator.Get<0>(id.id24), fenceAllocator.Get<1>(id.id24).fence) == VK_SUCCESS;
}

//------------------------------------------------------------------------------
/**
*/
bool 
FenceReset(const FenceId id)
{
	return vkResetFences(fenceAllocator.Get<0>(id.id24), 1, &fenceAllocator.Get<1>(id.id24).fence) == VK_SUCCESS;
}

//------------------------------------------------------------------------------
/**
*/
bool 
FenceWait(const FenceId id, const uint64 time)
{
	VkFence fence = fenceAllocator.Get<1>(id.id24).fence;
	VkDevice dev = fenceAllocator.Get<0>(id.id24);
	VkResult res = vkWaitForFences(dev, 1, &fence, false, time);
	return res == VK_SUCCESS || res == VK_TIMEOUT;
}

//------------------------------------------------------------------------------
/**
*/
bool 
FenceWaitAndReset(const FenceId id, const uint64 time)
{
	VkFence fence = fenceAllocator.Get<1>(id.id24).fence;
	VkDevice dev = fenceAllocator.Get<0>(id.id24);
	VkResult res = vkWaitForFences(dev, 1, &fence, false, time);
	if (res == VK_SUCCESS)
	{
		res = vkResetFences(dev, 1, &fence);
		n_assert(res == VK_SUCCESS);
	}
	return res == VK_SUCCESS || res == VK_TIMEOUT;
}

} // namespace CoreGraphics

