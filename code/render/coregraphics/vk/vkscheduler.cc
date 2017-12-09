//------------------------------------------------------------------------------
// vkscheduler.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkscheduler.h"
#include "vkrenderdevice.h"
#include "coregraphics/config.h"

namespace Vulkan
{
__ImplementSingleton(Vulkan::VkScheduler);
//------------------------------------------------------------------------------
/**
*/
VkScheduler::VkScheduler() :
	commands(NumCommandPasses)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkScheduler::~VkScheduler()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::PushImageLayoutTransition(VkDeferredCommand::CommandQueueType queue, VkImageMemoryBarrier barrier)
{
	VkDeferredCommand del;
	del.del.type = VkDeferredCommand::ImageLayoutTransition;
	del.del.imgBarrier.barrier = barrier;
	del.del.queue = queue;
	del.dev = VkRenderDevice::dev;
	this->PushCommand(del, OnBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::PushImageOwnershipChange(VkDeferredCommand::CommandQueueType queue, VkImageMemoryBarrier barrier)
{
	VkDeferredCommand del;
	del.del.type = VkDeferredCommand::ImageOwnershipChange;
	del.del.imgOwnerChange.barrier = barrier;
	del.del.queue = queue;
	del.dev = VkRenderDevice::dev;
	this->PushCommand(del, OnBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::PushImageColorClear(const VkImage& image, const VkDeferredCommand::CommandQueueType& queue, VkImageLayout layout, VkClearColorValue clearValue, VkImageSubresourceRange subres)
{
	VkDeferredCommand del;
	del.del.type = VkDeferredCommand::ClearColorImage;
	del.del.imgColorClear.clearValue = clearValue;
	del.del.imgColorClear.img = image;
	del.del.imgColorClear.layout = layout;
	del.del.imgColorClear.region = subres;
	del.del.queue = queue;
	del.dev = VkRenderDevice::dev;
	this->PushCommand(del, OnBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::PushImageDepthStencilClear(const VkImage& image, const VkDeferredCommand::CommandQueueType& queue, VkImageLayout layout, VkClearDepthStencilValue clearValue, VkImageSubresourceRange subres)
{
	VkDeferredCommand del;
	del.del.type = VkDeferredCommand::ClearDepthStencilImage;
	del.del.imgDepthStencilClear.clearValue = clearValue;
	del.del.imgDepthStencilClear.img = image;
	del.del.imgDepthStencilClear.layout = layout;
	del.del.imgDepthStencilClear.region = subres;
	del.del.queue = queue;
	del.dev = VkRenderDevice::dev;
	this->PushCommand(del, OnBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::PushImageUpdate(const VkImage& img, const VkImageCreateInfo& info, uint32_t mip, uint32_t face, VkDeviceSize size, uint32_t* data)
{
	uint32_t* imgCopy = (uint32_t*)Memory::Alloc(Memory::ScratchHeap, VK_DEVICE_SIZE_CONV(size));
	Memory::Copy(data, imgCopy, VK_DEVICE_SIZE_CONV(size));

	VkDeferredCommand del;
	del.del.type = VkDeferredCommand::UpdateImage;
	del.del.imageUpd.img = img;
	del.del.imageUpd.info = info;
	del.del.imageUpd.mip = mip;
	del.del.imageUpd.face = face;
	del.del.imageUpd.size = size;
	del.del.imageUpd.data = imgCopy;
	del.del.queue = VkDeferredCommand::Transfer;
	del.dev = VkRenderDevice::dev;
	this->PushCommand(del, OnBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::ExecuteCommandPass(const CommandPass& pass)
{
	if (pass == OnHandleDrawFences)
	{
		IndexT i;
		for (i = 0; i < this->drawFenceCommands.Size(); i++)
		{
			const VkFence& fence = this->drawFenceCommands.KeyAtIndex(i);
			VkResult res = vkGetFenceStatus(VkRenderDevice::dev, fence);
			if (res == VK_SUCCESS)
			{
				const Util::Array<VkDeferredCommand>& cmds = this->drawFenceCommands.ValueAtIndex(i);
				IndexT j;
				for (j = 0; j < cmds.Size(); j++)
				{
					cmds[j].RunDelegate();
				}
				vkDestroyFence(VkRenderDevice::dev, fence, NULL);
				this->drawFenceCommands.EraseAtIndex(i--);
			}
		}
	}
	else if (pass == OnHandleComputeFences)
	{
		IndexT i;
		for (i = 0; i < this->computeFenceCommands.Size(); i++)
		{
			const VkFence& fence = this->computeFenceCommands.KeyAtIndex(i);
			VkResult res = vkGetFenceStatus(VkRenderDevice::dev, fence);
			if (res == VK_SUCCESS)
			{
				const Util::Array<VkDeferredCommand>& cmds = this->computeFenceCommands.ValueAtIndex(i);
				IndexT j;
				for (j = 0; j < cmds.Size(); j++)
				{
					cmds[j].RunDelegate();
				}
				vkDestroyFence(VkRenderDevice::dev, fence, NULL);
				this->computeFenceCommands.EraseAtIndex(i--);
			}
		}
	}
	else if (pass == OnHandleTransferFences)
	{
		IndexT i;
		for (i = 0; i < this->transferFenceCommands.Size(); i++)
		{
			const VkFence& fence = this->transferFenceCommands.KeyAtIndex(i);
			VkResult res = vkGetFenceStatus(VkRenderDevice::dev, fence);
			if (res == VK_SUCCESS)
			{
				const Util::Array<VkDeferredCommand>& cmds = this->transferFenceCommands.ValueAtIndex(i);
				IndexT j;
				for (j = 0; j < cmds.Size(); j++)
				{
					cmds[j].RunDelegate();
				}
				vkDestroyFence(VkRenderDevice::dev, fence, NULL);
				this->transferFenceCommands.EraseAtIndex(i--);
			}
		}
	}
	else
	{
		const Util::Array<VkDeferredCommand>& cmds = this->commands[pass];
		IndexT i;
		for (i = 0; i < cmds.Size(); i++)
		{
			cmds[i].RunDelegate();
		}
		this->commands[pass].Clear();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::PushCommand(const VkDeferredCommand& cmd, const CommandPass& pass)
{
	if (pass == OnHandleDrawFences)				this->putDrawFenceThisFrame = true;
	else if (pass == OnHandleComputeFences)		this->putComputeFenceThisFrame = true;
	else if (pass == OnHandleTransferFences)	this->putTransferFenceThisFrame = true;
	this->commands[pass].Append(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::Begin()
{
	this->putDrawFenceThisFrame = false;
	this->putComputeFenceThisFrame = false;
	this->putTransferFenceThisFrame = false;
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::EndTransfers()
{
	const VkFenceCreateInfo info =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		0,
		0
	};
	if (this->putTransferFenceThisFrame)
	{
		VkFence fence;
		VkResult res = vkCreateFence(VkRenderDevice::dev, &info, NULL, &fence);
		n_assert(res == VK_SUCCESS);
		this->transferFenceCommands.Add(fence, this->commands[OnHandleTransferFences]);
		this->commands[OnHandleTransferFences].Clear();

		// submit to queue
		res = vkQueueSubmit(VkRenderDevice::transferQueue, 0, VK_NULL_HANDLE, fence);
		n_assert(res == VK_SUCCESS);
		this->putTransferFenceThisFrame = false;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::EndDraws()
{
	const VkFenceCreateInfo info =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		0,
		0
	};
	if (this->putDrawFenceThisFrame)
	{
		VkFence fence;
		VkResult res = vkCreateFence(VkRenderDevice::dev, &info, NULL, &fence);
		n_assert(res == VK_SUCCESS);
		this->drawFenceCommands.Add(fence, this->commands[OnHandleDrawFences]);
		this->commands[OnHandleDrawFences].Clear();
		res = vkQueueSubmit(VkRenderDevice::drawQueue, 0, VK_NULL_HANDLE, fence);
		n_assert(res == VK_SUCCESS);
		this->putDrawFenceThisFrame = false;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkScheduler::EndComputes()
{
	const VkFenceCreateInfo info =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		0,
		0
	};
	if (this->putComputeFenceThisFrame)
	{
		VkFence fence;
		VkResult res = vkCreateFence(VkRenderDevice::dev, &info, NULL, &fence);
		n_assert(res == VK_SUCCESS);
		this->computeFenceCommands.Add(fence, this->commands[OnHandleComputeFences]);
		this->commands[OnHandleComputeFences].Clear();
		res = vkQueueSubmit(VkRenderDevice::computeQueue, 0, VK_NULL_HANDLE, fence);
		n_assert(res == VK_SUCCESS);
		this->putComputeFenceThisFrame = false;
	}
}

} // namespace Vulkan