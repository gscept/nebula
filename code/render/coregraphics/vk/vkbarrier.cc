//------------------------------------------------------------------------------
// vkbarrier.cc
// (C) 2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkbarrier.h"
#include "coregraphics/config.h"
#include "vkcommandbuffer.h"
#include "vktypes.h"
#include "vkrendertexture.h"
#include "vkshaderrwtexture.h"
#include "vkshaderrwbuffer.h"
#include "coregraphics/vk/vkgraphicsdevice.h"

#if NEBULA_GRAPHICS_DEBUG
	#define NEBULA_BARRIER_INSERT_MARKER 1 // enable or disable to remove barrier markers
#else
	#define NEBULA_BARRIER_INSERT_MARKER 0
#endif
namespace Vulkan
{
VkBarrierAllocator barrierAllocator(0x00FFFFFF);
}

namespace CoreGraphics
{
using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
BarrierId
CreateBarrier(const BarrierCreateInfo& info)
{
	Ids::Id32 id = barrierAllocator.Alloc();
	VkBarrierInfo& vkInfo = barrierAllocator.Get<0>(id);
	Util::Array<CoreGraphics::RenderTextureId>& rts = barrierAllocator.Get<1>(id);
	Util::Array<CoreGraphics::ShaderRWTextureId>& rws = barrierAllocator.Get<2>(id);

	vkInfo.name = info.name;
	vkInfo.numImageBarriers = 0;
	vkInfo.numBufferBarriers = 0;
	vkInfo.numMemoryBarriers = 0;
	vkInfo.srcFlags = VkTypes::AsVkPipelineFlags(info.leftDependency);
	vkInfo.dstFlags = VkTypes::AsVkPipelineFlags(info.rightDependency);

	if (info.domain == BarrierDomain::Pass)
		vkInfo.dep = VK_DEPENDENCY_BY_REGION_BIT;

	n_assert(info.renderTextures.Size() < MaxNumBarriers);
	n_assert(info.rwTextures.Size() < MaxNumBarriers);
	n_assert(info.rwBuffers.Size() < MaxNumBarriers);

	IndexT i;
	for (i = 0; i < info.renderTextures.Size(); i++)
	{
		vkInfo.imageBarriers[vkInfo.numImageBarriers].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].pNext = nullptr;

		vkInfo.imageBarriers[vkInfo.numImageBarriers].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.renderTextures[i].fromAccess);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.renderTextures[i].toAccess);

		const ImageSubresourceInfo& subres = info.renderTextures[i].subres;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseMipLevel = subres.mip;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.levelCount = subres.mipCount;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseArrayLayer = subres.layer;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.layerCount = subres.layerCount;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].image = RenderTextureGetVkImage(info.renderTextures[i].tex);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].oldLayout = VkTypes::AsVkImageLayout(info.renderTextures[i].fromLayout);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].newLayout = VkTypes::AsVkImageLayout(info.renderTextures[i].toLayout);
		vkInfo.numImageBarriers++;

		rts.Append(info.renderTextures[i].tex);
	}

	// make sure we have room...
	n_assert(info.rwTextures.Size() < MaxNumBarriers - (SizeT)vkInfo.numImageBarriers);
	for (i = 0; i < info.rwTextures.Size(); i++)
	{
		vkInfo.imageBarriers[vkInfo.numImageBarriers].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].pNext = nullptr;

		vkInfo.imageBarriers[vkInfo.numImageBarriers].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwTextures[i].fromAccess);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwTextures[i].toAccess);

		const ImageSubresourceInfo& subres = info.rwTextures[i].subres;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseMipLevel = subres.mip;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.levelCount = subres.mipCount;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseArrayLayer = subres.layer;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.layerCount = subres.layerCount;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].image = ShaderRWTextureGetVkImage(info.rwTextures[i].tex);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].oldLayout = VkTypes::AsVkImageLayout(info.rwTextures[i].fromLayout);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].newLayout = VkTypes::AsVkImageLayout(info.rwTextures[i].toLayout);
		vkInfo.numImageBarriers++;

		rws.Append(info.rwTextures[i].tex);
	}

	for (i = 0; i < info.rwBuffers.Size(); i++)
	{
		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].pNext = nullptr;

		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].fromAccess);
		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].toAccess);

		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].buffer = ShaderRWBufferGetVkBuffer(info.rwBuffers[i].buf);
		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = 0;
		vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = VK_WHOLE_SIZE; 

		if (info.rwBuffers[i].size == -1)
		{
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = 0;
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = VK_WHOLE_SIZE;
		}
		else
		{
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = info.rwBuffers[i].offset;
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = info.rwBuffers[i].size;
		}

		vkInfo.numBufferBarriers++;
	}

	BarrierId eventId;
	eventId.id24 = id;
	eventId.id8 = BarrierIdType;
	return eventId;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyBarrier(const BarrierId id)
{
	barrierAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierInsert(const BarrierId id, const CoreGraphicsQueueType queue)
{
#if NEBULA_BARRIER_INSERT_MARKER
	const Util::StringAtom& name = barrierAllocator.Get<0>(id.id24).name;
	CommandBufferBeginMarker(queue, NEBULA_MARKER_GRAY, name.AsString());
#endif
	CoreGraphics::InsertBarrier(id, queue);

#if NEBULA_BARRIER_INSERT_MARKER
	CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierReset(const BarrierId id)
{
	VkBarrierInfo& vkInfo = barrierAllocator.Get<0>(id.id24);
	Util::Array<CoreGraphics::RenderTextureId>& rts = barrierAllocator.Get<1>(id.id24);
	Util::Array<CoreGraphics::ShaderRWTextureId>& rws = barrierAllocator.Get<2>(id.id24);

	IndexT i;
	for (i = 0; i < rts.Size(); i++)
	{
		vkInfo.imageBarriers[i].image = RenderTextureGetVkImage(rts[i]);
	}

	for (; i < rws.Size(); i++)
	{
		vkInfo.imageBarriers[i].image = ShaderRWTextureGetVkImage(rws[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
BarrierInsert(
	const CoreGraphicsQueueType queue, 
	CoreGraphics::BarrierStage fromStage,
	CoreGraphics::BarrierStage toStage,
	CoreGraphics::BarrierDomain domain,
	const Util::FixedArray<RenderTextureBarrier>& renderTextures, 
	const Util::FixedArray<BufferBarrier>& rwBuffers, 
	const Util::FixedArray<RWTextureBarrier>& rwTextures,
	const char* name)
{
	VkBarrierInfo barrier;
	barrier.name = name;
	barrier.srcFlags = VkTypes::AsVkPipelineFlags(fromStage);
	barrier.dstFlags = VkTypes::AsVkPipelineFlags(toStage);
	barrier.dep = domain == CoreGraphics::BarrierDomain::Pass ? VK_DEPENDENCY_BY_REGION_BIT : 0;
	barrier.numBufferBarriers = rwBuffers.Size();
	for (uint32_t i = 0; i < barrier.numBufferBarriers; i++)
	{
		VkBufferMemoryBarrier& vkBar = barrier.bufferBarriers[i];
		BufferBarrier& nebBar = rwBuffers[i];
		vkBar.buffer = ShaderRWBufferGetVkBuffer(nebBar.buf);
		vkBar.srcAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.fromAccess);
		vkBar.dstAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.toAccess);
		vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vkBar.pNext = nullptr;
		if (nebBar.size == -1)
		{
			vkBar.offset = 0;
			vkBar.size = VK_WHOLE_SIZE;
		}
		else
		{
			vkBar.offset = nebBar.offset;
			vkBar.size = nebBar.size;
		}
		
		vkBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		vkBar.pNext = nullptr;
		vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	}
	barrier.numImageBarriers = renderTextures.Size() + rwTextures.Size();
	IndexT i, j = 0;
	for (i = 0; i < renderTextures.Size(); i++, j++)
	{
		VkImageMemoryBarrier& vkBar = barrier.imageBarriers[j];
		RenderTextureBarrier& nebBar = renderTextures[i];

		vkBar.srcAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.fromAccess);
		vkBar.dstAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.toAccess);

		const ImageSubresourceInfo& subres = nebBar.subres;
		vkBar.subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkBar.subresourceRange.baseMipLevel = subres.mip;
		vkBar.subresourceRange.levelCount = subres.mipCount;
		vkBar.subresourceRange.baseArrayLayer = subres.layer;
		vkBar.subresourceRange.layerCount = subres.layerCount;
		vkBar.image = RenderTextureGetVkImage(nebBar.tex);
		vkBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBar.oldLayout = VkTypes::AsVkImageLayout(nebBar.fromLayout);
		vkBar.newLayout = VkTypes::AsVkImageLayout(nebBar.toLayout);

		vkBar.pNext = nullptr;
		vkBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	}
	for (i = 0; i < rwTextures.Size(); i++, j++)
	{
		VkImageMemoryBarrier& vkBar = barrier.imageBarriers[j];
		RWTextureBarrier& nebBar = rwTextures[i];

		vkBar.srcAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.fromAccess);
		vkBar.dstAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.toAccess);

		const ImageSubresourceInfo& subres = nebBar.subres;
		vkBar.subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkBar.subresourceRange.baseMipLevel = subres.mip;
		vkBar.subresourceRange.levelCount = subres.mipCount;
		vkBar.subresourceRange.baseArrayLayer = subres.layer;
		vkBar.subresourceRange.layerCount = subres.layerCount;
		vkBar.image = ShaderRWTextureGetVkImage(nebBar.tex);
		vkBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkBar.oldLayout = VkTypes::AsVkImageLayout(nebBar.fromLayout);
		vkBar.newLayout = VkTypes::AsVkImageLayout(nebBar.toLayout);

		vkBar.pNext = nullptr;
		vkBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	}
	barrier.numMemoryBarriers = 0; // maybe support this?

#if NEBULA_BARRIER_INSERT_MARKER
	CommandBufferBeginMarker(queue, NEBULA_MARKER_GRAY, name);
#endif
	Vulkan::InsertBarrier(barrier.srcFlags, barrier.dstFlags, barrier.dep, barrier.numMemoryBarriers, barrier.memoryBarriers, barrier.numBufferBarriers, barrier.bufferBarriers, barrier.numImageBarriers, barrier.imageBarriers, queue);

#if NEBULA_BARRIER_INSERT_MARKER
	CommandBufferEndMarker(queue);
#endif
}

} // namespace CoreGraphics