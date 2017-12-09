//------------------------------------------------------------------------------
//  vkcmdbuffer.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkcmdbuffer.h"
#include "vkrenderdevice.h"
#include "coregraphics/config.h"
namespace Vulkan
{

Ids::IdAllocator<VkCommandBuffer, VkCommandPool> commandBuffers(0x00FFFFFF);
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

	memset(pools.draw, 0, sizeof(pools.draw));
	memset(pools.compute, 0, sizeof(pools.compute));
	memset(pools.transfer, 0, sizeof(pools.transfer));
	memset(pools.sparse, 0, sizeof(pools.sparse));

	// draw pools, we assume we at least have drawing capabilities
	cmdPoolInfo.queueFamilyIndex = drawQueue;

	cmdPoolInfo.flags = 0;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.draw[0]);
	n_assert(result == VK_SUCCESS);

	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.draw[1]);
	n_assert(result == VK_SUCCESS);

	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.draw[2]);
	n_assert(result == VK_SUCCESS);

	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.draw[3]);
	n_assert(result == VK_SUCCESS);

	if (computeQueue != InvalidIndex)
	{
		// compute pools
		cmdPoolInfo.queueFamilyIndex = computeQueue;

		cmdPoolInfo.flags = 0;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.compute[0]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.compute[1]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.compute[2]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.compute[3]);
		n_assert(result == VK_SUCCESS);
	}

	if (transferQueue != InvalidIndex)
	{
		// transfer pools
		cmdPoolInfo.queueFamilyIndex = transferQueue;

		cmdPoolInfo.flags = 0;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.transfer[0]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.transfer[1]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.transfer[2]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.transfer[3]);
		n_assert(result == VK_SUCCESS);
	}

	if (sparseQueue != InvalidIndex)
	{
		// sparse
		cmdPoolInfo.queueFamilyIndex = sparseQueue;

		cmdPoolInfo.flags = 0;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.sparse[0]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.sparse[1]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.sparse[2]);
		n_assert(result == VK_SUCCESS);

		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		result = vkCreateCommandPool(dev, &cmdPoolInfo, NULL, &pools.sparse[3]);
		n_assert(result == VK_SUCCESS);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyVkPools(VkDevice dev)
{

	vkDestroyCommandPool(dev, pools.draw[0], nullptr);
	vkDestroyCommandPool(dev, pools.draw[1], nullptr);
	vkDestroyCommandPool(dev, pools.draw[2], nullptr);
	vkDestroyCommandPool(dev, pools.draw[3], nullptr);

	vkDestroyCommandPool(dev, pools.compute[0], nullptr);
	vkDestroyCommandPool(dev, pools.compute[1], nullptr);
	vkDestroyCommandPool(dev, pools.compute[2], nullptr);
	vkDestroyCommandPool(dev, pools.compute[3], nullptr);

	vkDestroyCommandPool(dev, pools.transfer[0], nullptr);
	vkDestroyCommandPool(dev, pools.transfer[1], nullptr);
	vkDestroyCommandPool(dev, pools.transfer[2], nullptr);
	vkDestroyCommandPool(dev, pools.transfer[3], nullptr);

	vkDestroyCommandPool(dev, pools.sparse[0], nullptr);
	vkDestroyCommandPool(dev, pools.sparse[1], nullptr);
	vkDestroyCommandPool(dev, pools.sparse[2], nullptr);
	vkDestroyCommandPool(dev, pools.sparse[3], nullptr);
}

//------------------------------------------------------------------------------
/**
*/
inline const VkCommandBuffer
CommandBufferGetVk(CmdBufferId id)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	return commandBuffers.Get<0>(id.id24);
}

} // Vulkan

namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
const CmdBufferId
CreateCmdBuffer(const CmdBufferCreateInfo& info)
{
	VkCommandPool* poolFamily;
	switch (info.usage)
	{
	case CmdDraw:
		poolFamily = pools.draw;
		break;
	case CmdCompute:
		poolFamily = pools.compute;
		break;
	case CmdTransfer:
		poolFamily = pools.transfer;
		break;
	case CmdSparse:
		poolFamily = pools.sparse;
		break;
	default:
		n_error("No Command Buffer usage %d exists!\n", info.usage);
	}

	// the array should match the flag values
	VkCommandPoolCreateFlags flags;
	flags |= info.resetable ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT : 0;
	flags |= info.shortlived ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;
	VkCommandPool pool = poolFamily[flags];
	VkCommandBufferAllocateInfo vkInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		pool,
		info.subBuffer ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		1
	};
	Ids::Id32 id = commandBuffers.AllocObject();
	vkAllocateCommandBuffers(pools.dev, &vkInfo, &commandBuffers.Get<0>(id));
	commandBuffers.Get<1>(id) = pool;
	return CmdBufferId{ id, CommandBufferIdType };
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyCmdBuffer(const CmdBufferId id)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	vkFreeCommandBuffers(pools.dev, commandBuffers.Get<1>(id.id24), 1, commandBuffers.Get<0>(id.id24));
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBufferBeginRecord(const CmdBufferId id, const CmdBufferBeginInfo& info)
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
	vkBeginCommandBuffer(commandBuffers.Get<0>(id.id24), &begin);
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBufferEndRecord(const CmdBufferId id)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	vkEndCommandBuffer(commandBuffers.Get<0>(id.id24));
}

//------------------------------------------------------------------------------
/**
*/
void
CmdBufferClear(const CmdBufferId id, const CmdBufferClearInfo& info)
{
#if _DEBUG
	n_assert(id.id8 == CommandBufferIdType);
#endif
	VkCommandBufferResetFlags flags = 0;
	flags |= info.allowRelease ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
	vkResetCommandBuffer(commandBuffers.Get<0>(id.id24), flags);
}

}
