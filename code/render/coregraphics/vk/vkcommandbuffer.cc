//------------------------------------------------------------------------------
//  vkcommandbuffer.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkcommandbuffer.h"
#include "coregraphics/config.h"
#include "vkgraphicsdevice.h"

namespace Vulkan
{

VkCommandBufferAllocator commandBuffers(0x00FFFFFF);
VkCommandBufferPoolAllocator commandBufferPools(0x00FFFFFF);
Threading::CriticalSection commandBufferCritSect;

//------------------------------------------------------------------------------
/**
*/
const VkCommandPool 
CommandBufferPoolGetVk(const CoreGraphics::CommandBufferPoolId id)
{
	return commandBufferPools.Get<CommandBufferPool_VkCommandPool>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkDevice 
CommandBufferPoolGetVkDevice(const CoreGraphics::CommandBufferPoolId id)
{
	return commandBufferPools.Get<CommandBufferPool_VkDevice>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const VkCommandBuffer
CommandBufferGetVk(const CoreGraphics::CommandBufferId id)
{
#if NEBULA_DEBUG
	n_assert(id.id8 == CoreGraphics::IdType::CommandBufferIdType);
#endif
	if (id == CoreGraphics::CommandBufferId::Invalid()) return VK_NULL_HANDLE;
	else												return commandBuffers.Get<CommandBuffer_VkCommandBuffer>(id.id24);
}

} // Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
const CommandBufferPoolId 
CreateCommandBufferPool(const CommandBufferPoolCreateInfo& info)
{
	Ids::Id32 id = commandBufferPools.Alloc();

	VkCommandPoolCreateFlags flags = 0;
	flags |= info.resetable ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;
	flags |= info.shortlived ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;

	uint32_t queueFamily = Vulkan::GetQueueFamily(info.queue);

	VkCommandPoolCreateInfo cmdPoolInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		flags,
		queueFamily
	};
	VkDevice dev = Vulkan::GetCurrentDevice();
	VkResult res = vkCreateCommandPool(dev, &cmdPoolInfo, nullptr, &commandBufferPools.Get<CommandBufferPool_VkCommandPool>(id));
	commandBufferPools.Set<CommandBufferPool_VkDevice>(id, dev);
	n_assert(res == VK_SUCCESS);

	CommandBufferPoolId ret;
	ret.id24 = id;
	ret.id8 = CommandBufferPoolIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyCommandBufferPool(const CommandBufferPoolId pool)
{
	vkDestroyCommandPool(commandBufferPools.Get<CommandBufferPool_VkDevice>(pool.id24), commandBufferPools.Get<CommandBufferPool_VkCommandPool>(pool.id24), nullptr);
}

//------------------------------------------------------------------------------
/**
*/
const CommandBufferId
CreateCommandBuffer(const CommandBufferCreateInfo& info)
{
	n_assert(info.pool != CoreGraphics::CommandBufferPoolId::Invalid());
	VkCommandPool pool = CommandBufferPoolGetVk(info.pool);
	VkCommandBufferAllocateInfo vkInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		pool,
		info.subBuffer ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};
	VkDevice dev = CommandBufferPoolGetVkDevice(info.pool);
	commandBufferCritSect.Enter();
	Ids::Id32 id = commandBuffers.Alloc();
	commandBufferCritSect.Leave();
	VkResult res = vkAllocateCommandBuffers(dev, &vkInfo, &commandBuffers.Get<CommandBuffer_VkCommandBuffer>(id));
	n_assert(res == VK_SUCCESS);
	commandBuffers.Get<CommandBuffer_VkCommandPool>(id) = pool;
	commandBuffers.Get<CommandBuffer_VkDevice>(id) = dev;
	CommandBufferId ret;
	ret.id24 = id;
	ret.id8 = CommandBufferIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyCommandBuffer(const CommandBufferId id)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	vkFreeCommandBuffers(commandBuffers.Get<CommandBuffer_VkDevice>(id.id24), commandBuffers.Get<CommandBuffer_VkCommandPool>(id.id24), 1, &commandBuffers.Get<CommandBuffer_VkCommandBuffer>(id.id24));
	commandBufferCritSect.Enter();
	commandBuffers.Dealloc(id.id24);
	commandBufferCritSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
CommandBufferBeginRecord(const CommandBufferId id, const CommandBufferBeginInfo& info)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	VkCommandBufferUsageFlags flags = 0;
	flags |= info.submitOnce ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
	flags |= info.submitDuringPass ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
	flags |= info.resubmittable ? VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : 0;
	VkCommandBufferBeginInfo begin =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		flags,
		nullptr		// fixme, this part can optimize if used properly!
	};
	VkResult res = vkBeginCommandBuffer(commandBuffers.Get<CommandBuffer_VkCommandBuffer>(id.id24), &begin);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
CommandBufferEndRecord(const CommandBufferId id)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	VkResult res = vkEndCommandBuffer(commandBuffers.Get<CommandBuffer_VkCommandBuffer>(id.id24));
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
CommandBufferClear(const CommandBufferId id, const CommandBufferClearInfo& info)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	VkCommandBufferResetFlags flags = 0;
	flags |= info.allowRelease ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;
	VkResult res = vkResetCommandBuffer(commandBuffers.Get<CommandBuffer_VkCommandBuffer>(id.id24), flags);
	n_assert(res == VK_SUCCESS);
}

} // namespace Vulkan
