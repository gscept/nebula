//------------------------------------------------------------------------------
// vkfence.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkcpusyncfence.h"
#include "vkrenderdevice.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkCpuSyncFence, 'VKFE', Base::BufferLockBase);
//------------------------------------------------------------------------------
/**
*/
VkCpuSyncFence::VkCpuSyncFence() :
	waitCmd(VK_NULL_HANDLE),
	signalCmd(VK_NULL_HANDLE)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkCpuSyncFence::~VkCpuSyncFence()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkCpuSyncFence::LockBuffer(IndexT index)
{
	if (!this->indexFences.Contains(index))
	{
		VkEventCreateInfo info =
		{
			VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
			NULL,
			0
		};
		VkEvent event;
		vkCreateEvent(VkRenderDevice::dev, &info, NULL, &event);
		this->indexFences.Add(index, event);

		// put event into signal command buffer
		vkCmdSetEvent(this->signalCmd, event, this->signalStage);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkCpuSyncFence::WaitForBuffer(IndexT index)
{
	if (this->indexFences.Contains(index))
	{
		this->Wait(this->indexFences[index]);
		this->Cleanup(this->indexFences[index]);
		this->indexFences.Erase(index);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkCpuSyncFence::LockRange(IndexT startIndex, SizeT length)
{
	Base::BufferRange range = { startIndex, length };
	VkEventCreateInfo info =
	{
		VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
		NULL,
		0
	};
	VkEvent event;
	vkCreateEvent(VkRenderDevice::dev, &info, NULL, &event);
	this->rangeFences.Add(range, event);

	// put event into signal command buffer
	vkCmdSetEvent(this->signalCmd, event, this->signalStage);
}

//------------------------------------------------------------------------------
/**
*/
void
VkCpuSyncFence::WaitForRange(IndexT startIndex, SizeT length)
{
	Base::BufferRange range = { startIndex, length };
	IndexT i;
	for (i = 0; i < this->rangeFences.Size(); i++)
	{
		const Util::KeyValuePair<Base::BufferRange, VkEvent>& kvp = this->rangeFences.KeyValuePairAtIndex(i);
		if (range.Overlaps(kvp.Key()))
		{
			this->Wait(kvp.Value());
			this->Cleanup(kvp.Value());
			this->rangeFences.Erase(kvp.Key());
			i--;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkCpuSyncFence::Wait(VkEvent sync)
{
	vkCmdWaitEvents(this->waitCmd, 1, &sync, this->waitStage, this->signalStage, 0, NULL, 0, NULL, 0, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
VkCpuSyncFence::Cleanup(VkEvent sync)
{
	vkDestroyEvent(VkRenderDevice::dev, sync, NULL);
}

} // namespace Vulkan
