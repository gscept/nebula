//------------------------------------------------------------------------------
//  vkevent.cc
//  (C)2017-2018 Individual contributors, see AUTHORS file
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
	fenceAllocator.Get<1>(id).pending = false;
	
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
void
FenceInsert(const FenceId id, const CoreGraphicsQueueType queue)
{
	CoreGraphics::SignalFence(id, queue);
}

//------------------------------------------------------------------------------
/**
*/
bool 
FencePeek(const FenceId id)
{
	return CoreGraphics::PeekFence(id);
}

//------------------------------------------------------------------------------
/**
*/

void 
FenceReset(const FenceId id)
{
	CoreGraphics::ResetFence(id);
}

//------------------------------------------------------------------------------
/**
*/
bool 
FenceWait(const FenceId id, const uint64 time)
{
	return CoreGraphics::WaitFence(id, time);
}

} // namespace CoreGraphics

