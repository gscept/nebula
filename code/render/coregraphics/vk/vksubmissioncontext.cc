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
#include "vktexture.h"
#include "vkbuffer.h"
#include "resources/resourceserver.h"

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
SubmissionContextFreeVkBuffer(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkBuffer buf)
{
	// get fence so we can wait for it
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContext_FreeBuffers>(id.id24)[currentIndex];
	buffers.Append(std::make_tuple(dev, buf));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeVkImage(const CoreGraphics::SubmissionContextId id, VkDevice dev, VkImage img)
{
	// get fence so we can wait for it
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContext_FreeImages>(id.id24)[currentIndex];
	images.Append(std::make_tuple(dev, img));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextSetTimelineIndex(const CoreGraphics::SubmissionContextId id, uint64 index)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	submissionContextAllocator.Get<SubmissionContext_TimelineIndex>(id.id24)[currentIndex] = index;
}

//------------------------------------------------------------------------------
/**
*/
uint64 
SubmissionContextGetTimelineIndex(const CoreGraphics::SubmissionContextId id)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	return submissionContextAllocator.Get<SubmissionContext_TimelineIndex>(id.id24)[currentIndex];
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;
void CleanupPendingDeletes(const CoreGraphics::SubmissionContextId id, IndexT currentIndex);

//------------------------------------------------------------------------------
/**
*/
SubmissionContextId
CreateSubmissionContext(const SubmissionContextCreateInfo& info)
{
	SubmissionContextId ret;
	Ids::Id32 id = submissionContextAllocator.Alloc();

	submissionContextAllocator.Get<SubmissionContext_NumCycles>(id) = info.numBuffers;
	submissionContextAllocator.Get<SubmissionContext_CmdBuffer>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_TimelineIndex>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_TimelineIndex>(id).Fill(0);
	submissionContextAllocator.Get<SubmissionContext_RetiredCmdBuffer>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_CmdCreateInfo>(id) = info.cmdInfo;
#if NEBULA_GRAPHICS_DEBUG
	submissionContextAllocator.Get<SubmissionContext_Name>(id) = info.name;
#endif

	submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id) = 0;
	submissionContextAllocator.Get<SubmissionContext_FreeResources>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_FreeBuffers>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_FreeImages>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_FreeCommandBuffers>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_ClearCommandBuffers>(id).Resize(info.numBuffers);
	submissionContextAllocator.Get<SubmissionContext_FreeMemories>(id).Resize(info.numBuffers);

	submissionContextAllocator.Get<SubmissionContext_FreeHostMemories>(id).Resize(info.numBuffers);

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
	const SizeT numCycles = submissionContextAllocator.Get<SubmissionContext_NumCycles>(id.id24);
	Util::FixedArray<CommandBufferId>& cmdBufs = submissionContextAllocator.Get<SubmissionContext_CmdBuffer>(id.id24);

	for (IndexT i = 0; i < numCycles; i++)
	{
		CleanupPendingDeletes(id, i);
	}
	
	submissionContextAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextNewBuffer(const SubmissionContextId id, CommandBufferId& outBuf)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	CommandBufferId& oldBuf = submissionContextAllocator.Get<SubmissionContext_CmdBuffer>(id.id24)[currentIndex];

	// append to retired buffers
	if (oldBuf != CommandBufferId::Invalid())
		submissionContextAllocator.Get<SubmissionContext_RetiredCmdBuffer>(id.id24)[currentIndex].Append(outBuf);

	// create new buffer and semaphore, we will delete the retired buffers upon next cycle when we come back
	CommandBufferCreateInfo cmdInfo = submissionContextAllocator.Get<SubmissionContext_CmdCreateInfo>(id.id24);
	outBuf = CreateCommandBuffer(cmdInfo);
	oldBuf = outBuf;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(outBuf, Util::String::Sprintf("%s Buffer %d", submissionContextAllocator.Get<SubmissionContext_Name>(id.id24).AsCharPtr(), currentIndex).AsCharPtr());
#endif
}

//------------------------------------------------------------------------------
/**
*/
CommandBufferId 
SubmissionContextGetCmdBuffer(const SubmissionContextId id)
{
	// get fence so we can wait for it
	IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	return submissionContextAllocator.Get<SubmissionContext_CmdBuffer>(id.id24)[currentIndex];
}

//------------------------------------------------------------------------------
/**
*/
void
SubmissionContextFreeResource(const CoreGraphics::SubmissionContextId id, const Resources::ResourceId res)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<Resources::ResourceId>& resources = submissionContextAllocator.Get<SubmissionContext_FreeResources>(id.id24)[currentIndex];
	resources.Append(res);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeTexture(const CoreGraphics::SubmissionContextId id, CoreGraphics::TextureId tex)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContext_FreeImages>(id.id24)[currentIndex];
	VkImage img = Vulkan::TextureGetVkImage(tex);
	VkDevice dev = Vulkan::TextureGetVkDevice(tex);
	images.Append(std::make_tuple(dev, img));
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeBuffer(const CoreGraphics::SubmissionContextId id, CoreGraphics::BufferId buf)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContext_FreeBuffers>(id.id24)[currentIndex];
	VkBuffer vkBuf = Vulkan::BufferGetVk(buf);
	VkDevice dev = Vulkan::BufferGetVkDevice(buf);
	buffers.Append(std::make_tuple(dev, vkBuf));
}

//------------------------------------------------------------------------------
/**
*/
void
SubmissionContextFreeCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<CoreGraphics::CommandBufferId>& buffers = submissionContextAllocator.Get<SubmissionContext_FreeCommandBuffers>(id.id24)[currentIndex];
	buffers.Append(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
SubmissionContextClearCommandBuffer(const CoreGraphics::SubmissionContextId id, const CoreGraphics::CommandBufferId cmd)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<CoreGraphics::CommandBufferId>& buffers = submissionContextAllocator.Get<SubmissionContext_ClearCommandBuffers>(id.id24)[currentIndex];
	buffers.Append(cmd);
}

//------------------------------------------------------------------------------
/**
*/
void
SubmissionContextFreeMemory(const CoreGraphics::SubmissionContextId id, const CoreGraphics::Alloc& alloc)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<CoreGraphics::Alloc>& mem = submissionContextAllocator.Get<SubmissionContext_FreeMemories>(id.id24)[currentIndex];
	mem.Append(alloc);
}

//------------------------------------------------------------------------------
/**
*/
void 
SubmissionContextFreeHostMemory(const SubmissionContextId id, void* buf)
{
	const IndexT currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);
	Util::Array<void*>& memories = submissionContextAllocator.Get<SubmissionContext_FreeHostMemories>(id.id24)[currentIndex];
	memories.Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
const FenceId 
SubmissionContextGetFence(const SubmissionContextId id)
{
	// for vulkan, we use timeline
	return FenceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
const FenceId
SubmissionContextNextCycle(const SubmissionContextId id, const std::function<void()>& sync)
{
	// get fence so we can wait for it
	IndexT& currentIndex = submissionContextAllocator.Get<SubmissionContext_CurrentIndex>(id.id24);

	// cycle index and update
	currentIndex = (currentIndex + 1) % submissionContextAllocator.Get<SubmissionContext_NumCycles>(id.id24);

	// run sync function
	if (sync)
		sync();

	// clean up retired buffers and semaphores
	Util::Array<CommandBufferId>& bufs = submissionContextAllocator.Get<SubmissionContext_RetiredCmdBuffer>(id.id24)[currentIndex];

	for (IndexT i = 0; i < bufs.Size(); i++)
	{
		DestroyCommandBuffer(bufs[i]);
	}
	bufs.Clear();

	// also destroy current buffers
	CommandBufferId& buf = submissionContextAllocator.Get<SubmissionContext_CmdBuffer>(id.id24)[currentIndex];
	if (buf != CommandBufferId::Invalid())
	{
		DestroyCommandBuffer(buf);
		buf = CommandBufferId::Invalid();
	}

	CleanupPendingDeletes(id, currentIndex);

	return FenceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
SubmissionContextPoll(const SubmissionContextId id, const std::function<bool(uint64)>& sync)
{
	SizeT numCycles = submissionContextAllocator.Get<SubmissionContext_NumCycles>(id.id24);
	for (IndexT i = 0; i < numCycles; i++)
	{
		uint64 index = submissionContextAllocator.Get<SubmissionContext_TimelineIndex>(id.id24)[i];
		if (index != 0 && sync(index))
		{
			Util::Array<CommandBufferId>& bufs = submissionContextAllocator.Get<SubmissionContext_RetiredCmdBuffer>(id.id24)[i];
			for (IndexT j = 0; j < bufs.Size(); j++)
			{
				DestroyCommandBuffer(bufs[j]);
			}
			bufs.Clear();
			CleanupPendingDeletes(id, i);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
CleanupPendingDeletes(const SubmissionContextId id, IndexT currentIndex)
{
	// delete any pending resources this context has allocated
	Util::Array<Resources::ResourceId>& resources = submissionContextAllocator.Get<SubmissionContext_FreeResources>(id.id24)[currentIndex];
	for (IndexT i = 0; i < resources.Size(); i++)
		//Resources::DiscardResource(resources[i]);
	resources.Clear();

	Util::Array<std::tuple<VkDevice, VkBuffer>>& buffers = submissionContextAllocator.Get<SubmissionContext_FreeBuffers>(id.id24)[currentIndex];
	for (IndexT i = 0; i < buffers.Size(); i++)
		vkDestroyBuffer(std::get<0>(buffers[i]), std::get<1>(buffers[i]), nullptr);
	buffers.Clear();

	Util::Array<std::tuple<VkDevice, VkImage>>& images = submissionContextAllocator.Get<SubmissionContext_FreeImages>(id.id24)[currentIndex];
	for (IndexT i = 0; i < images.Size(); i++)
		vkDestroyImage(std::get<0>(images[i]), std::get<1>(images[i]), nullptr);
	images.Clear();

	Util::Array<CoreGraphics::CommandBufferId>& freeCommandBuffers = submissionContextAllocator.Get<SubmissionContext_FreeCommandBuffers>(id.id24)[currentIndex];
	for (IndexT i = 0; i < freeCommandBuffers.Size(); i++)
		CoreGraphics::DestroyCommandBuffer(freeCommandBuffers[i]);
	freeCommandBuffers.Clear();

	Util::Array<CoreGraphics::CommandBufferId>& clearCommandBuffers = submissionContextAllocator.Get<SubmissionContext_ClearCommandBuffers>(id.id24)[currentIndex];
	for (IndexT i = 0; i < clearCommandBuffers.Size(); i++)
		CoreGraphics::CommandBufferClear(clearCommandBuffers[i], CommandBufferClearInfo{ false });
	clearCommandBuffers.Clear();

	Util::Array<void*>& hostMemories = submissionContextAllocator.Get<SubmissionContext_FreeHostMemories>(id.id24)[currentIndex];
	for (IndexT i = 0; i < hostMemories.Size(); i++)
		Memory::Free(Memory::ScratchHeap, hostMemories[i]);
	hostMemories.Clear();

	Util::Array<CoreGraphics::Alloc>& mem = submissionContextAllocator.Get<SubmissionContext_FreeMemories>(id.id24)[currentIndex];
	for (IndexT i = 0; i < mem.Size(); i++)
		FreeMemory(mem[i]);
	mem.Clear();
}

} // namespace CoreGraphics

#pragma pop_macro("CreateSemaphore")
