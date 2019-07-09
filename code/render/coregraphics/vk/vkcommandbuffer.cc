//------------------------------------------------------------------------------
//  vkcommandbuffer.cc
//  (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkcommandbuffer.h"
#include "coregraphics/config.h"
namespace Vulkan
{

VkCommandBufferAllocator commandBuffers(0x00FFFFFF);
CommandBufferPools pools;

//------------------------------------------------------------------------------
/**
*/
void
SetupVkPools(VkDevice dev, uint32_t drawQueue, uint32_t computeQueue, uint32_t transferQueue, uint32_t sparseQueue)
{
	VkResult result;

	// create command pool for graphics
	VkCommandPoolCreateInfo cmdPoolInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		NULL,
		0,
		0
	};

	pools.dev = dev;

	memset(pools.pools[CoreGraphics::CommandGfx], 0, sizeof(pools.pools[CoreGraphics::CommandGfx]));
	memset(pools.pools[CoreGraphics::CommandCompute], 0, sizeof(pools.pools[CoreGraphics::CommandCompute]));
	memset(pools.pools[CoreGraphics::CommandTransfer], 0, sizeof(pools.pools[CoreGraphics::CommandTransfer]));
	memset(pools.pools[CoreGraphics::CommandSparse], 0, sizeof(pools.pools[CoreGraphics::CommandSparse]));
	pools.queueFamilies[CoreGraphics::CommandGfx] = drawQueue;
	pools.queueFamilies[CoreGraphics::CommandCompute] = computeQueue;
	pools.queueFamilies[CoreGraphics::CommandTransfer] = transferQueue;
	pools.queueFamilies[CoreGraphics::CommandSparse] = sparseQueue;

	// draw pools, we assume we at least have drawing capabilities
	cmdPoolInfo.queueFamilyIndex = drawQueue;

	cmdPoolInfo.flags = 0;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandGfx][cmdPoolInfo.flags]);
	n_assert(result == VK_SUCCESS);

	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandGfx][cmdPoolInfo.flags]);
	n_assert(result == VK_SUCCESS);

	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandGfx][cmdPoolInfo.flags]);
	n_assert(result == VK_SUCCESS);

	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandGfx][cmdPoolInfo.flags]);
	n_assert(result == VK_SUCCESS);

	if (computeQueue != InvalidIndex)
	{
		// compute pools
		cmdPoolInfo.queueFamilyIndex = computeQueue;

		cmdPoolInfo.flags = 0;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandCompute][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandCompute][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandCompute][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandCompute][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);
	}

	if (transferQueue != InvalidIndex)
	{
		// transfer pools
		cmdPoolInfo.queueFamilyIndex = transferQueue;

		cmdPoolInfo.flags = 0;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandTransfer][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandTransfer][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandTransfer][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandTransfer][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);
	}

	if (sparseQueue != InvalidIndex)
	{
		// sparse
		cmdPoolInfo.queueFamilyIndex = sparseQueue;

		cmdPoolInfo.flags = 0;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandSparse][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandSparse][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandSparse][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.pools[CoreGraphics::CommandSparse][cmdPoolInfo.flags]);
		n_assert(result == VK_SUCCESS);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyVkPools(VkDevice dev)
{
	for (IndexT i = 0; i < CoreGraphics::NumCommandBufferUsages; i++)
	{
		vkDestroyCommandPool(dev, pools.pools[i][0], nullptr);
		vkDestroyCommandPool(dev, pools.pools[i][1], nullptr);
		vkDestroyCommandPool(dev, pools.pools[i][2], nullptr);
		vkDestroyCommandPool(dev, pools.pools[i][3], nullptr);
	}
}

//------------------------------------------------------------------------------
/**
*/
const VkCommandPool
CommandBufferGetVkPool(CoreGraphics::CommandBufferUsage usage, VkCommandPoolCreateFlags flags)
{
	n_assert(usage != CoreGraphics::InvalidCommandUsage);
	return pools.pools[usage][flags];
}

//------------------------------------------------------------------------------
/**
*/
const VkCommandBuffer
CommandBufferGetVk(const CoreGraphics::CommandBufferId id)
{
#if NEBULA_DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	if (id == CoreGraphics::CommandBufferId::Invalid()) return VK_NULL_HANDLE;
	else												return commandBuffers.Get<0>(id.id24);
}

} // Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
const CommandBufferId
CreateCommandBuffer(const CommandBufferCreateInfo& info)
{
	VkCommandPoolCreateFlags flags = 0;
	flags |= info.resetable ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;
	flags |= info.shortlived ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;

	n_assert(info.usage != CoreGraphics::InvalidCommandUsage);
	VkCommandPool pool = pools.pools[info.usage][flags];

	VkCommandBufferAllocateInfo vkInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		pool,
		info.subBuffer ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};
	Ids::Id32 id = commandBuffers.Alloc();
	VkResult res = vkAllocateCommandBuffers(pools.dev, &vkInfo, &commandBuffers.Get<0>(id));
	n_assert(res == VK_SUCCESS);
	commandBuffers.Get<1>(id) = pool;
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
	vkFreeCommandBuffers(pools.dev, commandBuffers.Get<1>(id.id24), 1, &commandBuffers.Get<0>(id.id24));
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
	VkResult res = vkBeginCommandBuffer(commandBuffers.Get<0>(id.id24), &begin);
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
	VkResult res = vkEndCommandBuffer(commandBuffers.Get<0>(id.id24));
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
	VkResult res = vkResetCommandBuffer(commandBuffers.Get<0>(id.id24), flags);
	n_assert(res == VK_SUCCESS);
}

} // namespace Vulkan
