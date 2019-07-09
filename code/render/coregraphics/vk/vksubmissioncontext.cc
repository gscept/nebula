//------------------------------------------------------------------------------
//  vksubmissioncontext.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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
	Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContextFreeBuffers>(id.id24);
	buffers.Append(std::make_tuple(dev, buf));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeDeviceMemory(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkDeviceMemory mem)
{
	Util::Array<std::tuple<VkDevice, VkDeviceMemory>>& memories = submissionContextAllocator.Get<SubmissionContextFreeDeviceMemories>(id.id24);
	memories.Append(std::make_tuple(dev, mem));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeImage(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImage img)
{
	Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContextFreeImages>(id.id24);
	images.Append(std::make_tuple(dev, img));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeCommandBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkCommandPool pool, VkCommandBuffer buf)
{
	Util::Array<std::tuple<VkDevice, VkCommandPool, VkCommandBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContextFreeCommandBuffers>(id.id24);
	buffers.Append(std::make_tuple(dev, pool, buf));
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
	submissionContextAllocator.Get<SubmissionContextSemaphore>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContextRetiredSemaphore>(id).Resize(info.numBuffers);
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
	const Util::FixedArray<CommandBufferId>& bufs = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24);
	const Util::FixedArray<SemaphoreId>& sems = submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24);

	// go through buffers and semaphores and clear the ones created
	for (IndexT i = 0; i < bufs.Size(); i++)
	{
		DestroyCommandBuffer(bufs[i]);
		DestroySemaphore(sems[i]);
	}
	submissionContextAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextNewBuffer(const SubmissionContextId id, CommandBufferId& outBuf, SemaphoreId& outSem)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	CommandBufferId& oldBuf = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex];
	SemaphoreId& oldSem = submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24)[currentIndex];

	// append to retired buffers
	if (oldBuf != CommandBufferId::Invalid())
		submissionContextAllocator.Get<SubmissionContextRetiredCmdBuffer>(id.id24)[currentIndex].Append(outBuf);
	if (oldSem != SemaphoreId::Invalid())
		submissionContextAllocator.Get<SubmissionContextRetiredSemaphore>(id.id24)[currentIndex].Append(outSem);

	// create new buffer and semaphore, we will delete the retired buffers upon next cycle when we come back
	CommandBufferCreateInfo cmdInfo = submissionContextAllocator.Get<SubmissionContextCmdCreateInfo>(id.id24);
	outBuf = CreateCommandBuffer(cmdInfo);
	oldBuf = outBuf;
	SemaphoreCreateInfo semInfo{};
	outSem = CreateSemaphore(semInfo);
	oldSem = outSem;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(outBuf, Util::String::Sprintf("%s Buffer %d", submissionContextAllocator.Get<SubmissionContextName>(id.id24).AsCharPtr(), currentIndex));
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
SemaphoreId 
SubmissionContextGetSemaphore(const SubmissionContextId id)
{
	// get fence so we can wait for it
	IndexT currentIndex = submissionContextAllocator.Get<SubmissionContextCurrentIndex>(id.id24);
	return submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24)[currentIndex];
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeHostMemory(const SubmissionContextId id, void* buf)
{
	Util::Array<void*>& memories = submissionContextAllocator.Get<SubmissionContextFreeHostMemories>(id.id24);
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
	Util::Array<SemaphoreId>& sems = submissionContextAllocator.Get<SubmissionContextRetiredSemaphore>(id.id24)[currentIndex];

	for (IndexT i = 0; i < bufs.Size(); i++)
	{
		DestroyCommandBuffer(bufs[i]);
		DestroySemaphore(sems[i]);
	}
	bufs.Clear();
	sems.Clear();

	// also destroy current buffers
	CommandBufferId& buf = submissionContextAllocator.Get<SubmissionContextCmdBuffer>(id.id24)[currentIndex];
	SemaphoreId& sem = submissionContextAllocator.Get<SubmissionContextSemaphore>(id.id24)[currentIndex];
	if (buf != CommandBufferId::Invalid())
	{
		DestroyCommandBuffer(buf);
		buf = CommandBufferId::Invalid();
	}
	if (sem != SemaphoreId::Invalid())
	{
		DestroySemaphore(sem);
		sem = SemaphoreId::Invalid();
	}

	// delete any pending resources this context has allocated
	Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContextFreeBuffers>(id.id24);
	for (IndexT i = 0; i < buffers.Size(); i++)
		vkDestroyBuffer(std::get<0>(buffers[i]), std::get<1>(buffers[i]), nullptr);
	buffers.Clear();

	Util::Array<std::tuple<VkDevice, VkDeviceMemory>>& memories = submissionContextAllocator.Get<SubmissionContextFreeDeviceMemories>(id.id24);
	for (IndexT i = 0; i < memories.Size(); i++)
		vkFreeMemory(std::get<0>(memories[i]), std::get<1>(memories[i]), nullptr);
	memories.Clear();

	Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContextFreeImages>(id.id24);
	for (IndexT i = 0; i < images.Size(); i++)
		vkDestroyImage(std::get<0>(images[i]), std::get<1>(images[i]), nullptr);
	images.Clear();

	Util::Array<std::tuple<VkDevice, VkCommandPool, VkCommandBuffer>>& commandBuffers = submissionContextAllocator.Get<SubmissionContextFreeCommandBuffers>(id.id24);
	for (IndexT i = 0; i < commandBuffers.Size(); i++)
		vkFreeCommandBuffers(std::get<0>(commandBuffers[i]), std::get<1>(commandBuffers[i]), 1, &std::get<2>(commandBuffers[i]));
	commandBuffers.Clear();

	return nextFence;
}

} // namespace CoreGraphics

#pragma pop_macro("CreateSemaphore")