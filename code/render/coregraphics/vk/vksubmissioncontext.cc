//------------------------------------------------------------------------------
//  vksubmissioncontext.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vksubmissioncontext.h"
#include "coregraphics/commandbuffer.h"
#include "coregraphics/semaphore.h"
#include "coregraphics/fence.h"
#include "coregraphics/graphicsdevice.h"

#ifdef CreateSemaphore
#pragma push_macro("CreateSemaphore")
#undef CreateSemaphore
#endif


namespace Vulkan
{
SubmissionContextAllocator submissionContextAllocator;

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkBuffer buf)
{
	// get fence so we can wait for it
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContextFreeBuffers>(id.id24)[currentIndex];
	buffers.Append(std::make_tuple(dev, buf));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeDeviceMemory(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkDeviceMemory mem)
{
	// get fence so we can wait for it
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	Util::Array<std::tuple<VkDevice, VkDeviceMemory>>& memories = submissionContextAllocator.Get<SubmissionContextFreeDeviceMemories>(id.id24)[currentIndex];
	memories.Append(std::make_tuple(dev, mem));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeImage(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImage img)
{
	// get fence so we can wait for it
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContextFreeImages>(id.id24)[currentIndex];
	images.Append(std::make_tuple(dev, img));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	Util::Array<CoreGraphics::CommandBufferId>& buffers = submissionContextAllocator.Get<SubmissionContextFreeCommandBuffers>(id.id24)[currentIndex];
	buffers.Append(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextClearCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	Util::Array<CoreGraphics::CommandBufferId>& buffers = submissionContextAllocator.Get<SubmissionContextClearCommandBuffers>(id.id24)[currentIndex];
	buffers.Append(cmd);
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
SubmissionContextId
CreateSubmissionContext(const SubmissionContextCreateInfo& info)
{
	SubmissionContextId ret;
	Ids::Id32 id = submissionContextAllocator.Alloc();

	submissionContextAllocator.Get<SubmissionContextNumCycles>(id) = info.numBuffers;
	submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextCmdCreateInfo>(id) = info.cmdInfo;
#if NEBULA_GRAPHICS_DEBUG
	submissionContextAllocator.Get<SubmissionContextName>(id) = info.name;
#endif

	// create fences
	if (info.useFence)
	{
		submissionContextAllocator.Get<SubmissionContextFence>(id).Resize(info.numBuffers);
		for (uint i = 0; i < info.numBuffers; i++)
		{
			FenceId& fence = submissionContextAllocator.Get<SubmissionContextFence>(id)[i];

			FenceCreateInfo fenceInfo{ true };
			fence = CreateFence(fenceInfo);
		}
	}	

	submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id) = 0;
	submissionContextAllocator.Get<SubmissionContextFreeBuffers>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextFreeDeviceMemories>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextFreeImages>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextFreeCommandBuffers>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextClearCommandBuffers>(id).Resize(info.numBuffers);

	submissionContextAllocator.Get<SubmissionContextFreeHostMemories>(id).Resize(info.numBuffers);

	ret.id24 = id;
	ret.id8 = SubmissionContextIdType;
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroySubmissionContext(const SubmissionContextId id)
{
	const SizeT numCycles = submissionContextAllocator.Get<SubmissionContextNumCycles>(id.id24);
	Util::FixedArray<FenceId>& fences = submissionContextAllocator.Get<SubmissionContextFence>(id.id24);
	Util::FixedArray<CommandBufferId>& cmdBufs = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24);

	for (IndexT i = 0; i < numCycles; i++)
	{
		// clear up fences
		if (fences.Size() > 0)
			DestroyFence(fences[i]);

		// clear up all current command buffers (should only be one)
		if (cmdBufs[i] != CommandBufferId::Invalid())
			DestroyCommandBuffer(cmdBufs[i]);

		// clear up all retired buffers which might be in flight
		Util::Array<CommandBufferId>& bufs = submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id.id24)[i];
		for (IndexT j = 0; j < bufs.Size(); j++)
			DestroyCommandBuffer(bufs[j]);
		bufs.Clear();

		// delete any pending resources this context has allocated
		Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContextFreeBuffers>(id.id24)[i];
		for (IndexT j = 0; j < buffers.Size(); j++)
			vkDestroyBuffer(std::get<0>(buffers[j]), std::get<1>(buffers[j]), nullptr);
		buffers.Clear();

		Util::Array<std::tuple<VkDevice, VkDeviceMemory>>& memories = submissionContextAllocator.Get<SubmissionContextFreeDeviceMemories>(id.id24)[i];
		for (IndexT j = 0; j < memories.Size(); j++)
			vkFreeMemory(std::get<0>(memories[j]), std::get<1>(memories[j]), nullptr);
		memories.Clear();

		Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContextFreeImages>(id.id24)[i];
		for (IndexT j = 0; j < images.Size(); j++)
			vkDestroyImage(std::get<0>(images[j]), std::get<1>(images[j]), nullptr);
		images.Clear();

		Util::Array<CoreGraphics::CommandBufferId>& commandBuffers = submissionContextAllocator.Get<SubmissionContextFreeCommandBuffers>(id.id24)[i];
		for (IndexT j = 0; j < commandBuffers.Size(); j++)
			CoreGraphics::DestroyCommandBuffer(commandBuffers[j]);
		commandBuffers.Clear();

		submissionContextAllocator.Get<SubmissionContextClearCommandBuffers>(id.id24)[i].Clear();

		Util::Array<void*>& hostMemories = submissionContextAllocator.Get<SubmissionContextFreeHostMemories>(id.id24)[i];
		for (IndexT j = 0; j < hostMemories.Size(); j++)
			Memory::Free(Memory::ScratchHeap, hostMemories[j]);
		hostMemories.Clear();
	}
	
	submissionContextAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextNewBuffer(const SubmissionContextId id, CommandBufferId& outBuf)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	CommandBufferId& oldBuf = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex];

	// append to retired buffers
	if (oldBuf != CommandBufferId::Invalid())
		submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id.id24)[currentIndex].Append(outBuf);

	// create new buffer and semaphore, we will delete the retired buffers upon next cycle when we come back
	CommandBufferCreateInfo cmdInfo = submissionContextAllocator.Get<SubmissionContextCmdCreateInfo>(id.id24);
	outBuf = CreateCommandBuffer(cmdInfo);
	oldBuf = outBuf;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(outBuf, Util::String::Sprintf("%s Buffer %d", submissionContextAllocator.Get<SubmissionContextName>(id.id24).AsCharPtr(), currentIndex).AsCharPtr());
#endif
}

//------------------------------------------------------------------------------
/**
*/
CommandBufferId 
SubmissionContextGetCmdBuffer(const SubmissionContextId id)
{
	// get fence so we can wait for it
	IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	return submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex];
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeHostMemory(const SubmissionContextId id, void* buf)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	Util::Array<void*>& memories = submissionContextAllocator.Get<SubmissionContextFreeHostMemories>(id.id24)[currentIndex];
	memories.Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
const FenceId 
SubmissionContextGetFence(const SubmissionContextId id)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	return submissionContextAllocator.Get<SubmissionContextFence>(id.id24)[currentIndex];
}

//------------------------------------------------------------------------------
/**
*/
const FenceId
SubmissionContextNextCycle(const SubmissionContextId id)
{
	// get fence so we can wait for it
	IndexT& currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);

	// cycle index and update
	currentIndex = (currentIndex + 1) % submissionContextAllocator.Get<SubmissionContextNumCycles>(id.id24);

	// get next fence and wait for it to finish
	auto& fences = submissionContextAllocator.Get<SubmissionContextFence>(id.id24);
	FenceId nextFence = FenceId::Invalid();
	if (fences.Size() > 0)
	{
		nextFence = submissionContextAllocator.Get<SubmissionContextFence>(id.id24)[currentIndex];
		bool res = FenceWait(nextFence, UINT64_MAX);
		n_assert(res);
	}	

	// clean up retired buffers and semaphores
	Util::Array<CommandBufferId>& bufs = submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id.id24)[currentIndex];

	for (IndexT i = 0; i < bufs.Size(); i++)
	{
		DestroyCommandBuffer(bufs[i]);
	}
	bufs.Clear();

	// also destroy current buffers
	CommandBufferId& buf = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex];
	if (buf != CommandBufferId::Invalid())
	{
		DestroyCommandBuffer(buf);
		buf = CommandBufferId::Invalid();
	}

	// delete any pending resources this context has allocated
	Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContextFreeBuffers>(id.id24)[currentIndex];
	for (IndexT i = 0; i < buffers.Size(); i++)
		vkDestroyBuffer(std::get<0>(buffers[i]), std::get<1>(buffers[i]), nullptr);
	buffers.Clear();

	Util::Array<std::tuple<VkDevice, VkDeviceMemory>>& memories = submissionContextAllocator.Get<SubmissionContextFreeDeviceMemories>(id.id24)[currentIndex];
	for (IndexT i = 0; i < memories.Size(); i++)
		vkFreeMemory(std::get<0>(memories[i]), std::get<1>(memories[i]), nullptr);
	memories.Clear();

	Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContextFreeImages>(id.id24)[currentIndex];
	for (IndexT i = 0; i < images.Size(); i++)
		vkDestroyImage(std::get<0>(images[i]), std::get<1>(images[i]), nullptr);
	images.Clear();

	Util::Array<CoreGraphics::CommandBufferId>& freeCommandBuffers = submissionContextAllocator.Get<SubmissionContextFreeCommandBuffers>(id.id24)[currentIndex];
	for (IndexT i = 0; i < freeCommandBuffers.Size(); i++)
		CoreGraphics::DestroyCommandBuffer(freeCommandBuffers[i]);
	freeCommandBuffers.Clear();

	Util::Array<CoreGraphics::CommandBufferId>& clearCommandBuffers = submissionContextAllocator.Get<SubmissionContextClearCommandBuffers>(id.id24)[currentIndex];
	for (IndexT i = 0; i < clearCommandBuffers.Size(); i++)
		CoreGraphics::CommandBufferClear(clearCommandBuffers[i], CommandBufferClearInfo{ false });
	clearCommandBuffers.Clear();

	Util::Array<void*>& hostMemories = submissionContextAllocator.Get<SubmissionContextFreeHostMemories>(id.id24)[currentIndex];
	for (IndexT i = 0; i < hostMemories.Size(); i++)
		Memory::Free(Memory::ScratchHeap, hostMemories[i]);
	hostMemories.Clear();

	return nextFence;
}

} // namespace CoreGraphics

#pragma pop_macro("CreateSemaphore")