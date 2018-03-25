//------------------------------------------------------------------------------
// vkbarrier.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkbarrier.h"
#include "coregraphics/config.h"
#include "vkcmdbuffer.h"
#include "vktypes.h"
#include "vkrendertexture.h"
#include "vkshaderrwtexture.h"
#include "vkshaderrwbuffer.h"

namespace CoreGraphics
{
using namespace Vulkan;
VkBarrierAllocator barrierAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
BarrierId
CreateBarrier(const BarrierCreateInfo& info)
{
	Ids::Id32 id = barrierAllocator.AllocObject();
	VkBarrierInfo& vkInfo = barrierAllocator.Get<0>(id);
	vkInfo.numImageBarriers = 0;
	vkInfo.numBufferBarriers = 0;
	vkInfo.numMemoryBarriers = 0;
	vkInfo.srcFlags = VkTypes::AsVkPipelineFlags(info.leftDependency);
	vkInfo.dstFlags = VkTypes::AsVkPipelineFlags(info.rightDependency);

	if (info.domain == BarrierDomain::Pass)
		vkInfo.dep = VK_DEPENDENCY_BY_REGION_BIT;

	n_assert(info.renderTextureBarriers.Size() < MaxNumBarriers);
	n_assert(info.shaderRWTextures.Size() < MaxNumBarriers);
	n_assert(info.shaderRWBuffers.Size() < MaxNumBarriers);

	IndexT i;
	for (i = 0; i < info.renderTextureBarriers.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(info.renderTextureBarriers[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<2>(info.renderTextureBarriers[i]));

		vkInfo.imageBarriers[i].image = RenderTextureGetVkImage(std::get<0>(info.renderTextureBarriers[i]));
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.numImageBarriers++;
	}

	// make sure we have room...
	n_assert(info.shaderRWTextures.Size() < MaxNumBarriers - i);
	for (; i < info.shaderRWTextures.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<2>(info.shaderRWTextures[i]));

		vkInfo.imageBarriers[i].image = ShaderRWTextureGetVkImage(std::get<0>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.numImageBarriers++;
	}

	for (i = 0; i < info.shaderRWBuffers.Size(); i++)
	{
		vkInfo.bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vkInfo.bufferBarriers[i].pNext = nullptr;

		vkInfo.bufferBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(info.shaderRWBuffers[i]));
		vkInfo.bufferBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<2>(info.shaderRWBuffers[i]));

		vkInfo.bufferBarriers[i].buffer = ShaderRWBufferGetVkBuffer(std::get<0>(info.shaderRWBuffers[i]));
		vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
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
	barrierAllocator.DeallocObject(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
InsertBarrier(const BarrierId id, const CmdBufferId cmd)
{
	const VkBarrierInfo& vkInfo = barrierAllocator.Get<0>(id.id24);
	vkCmdPipelineBarrier(CommandBufferGetVk(cmd), vkInfo.srcFlags, vkInfo.dstFlags, vkInfo.dep, vkInfo.numMemoryBarriers, vkInfo.memoryBarriers, vkInfo.numBufferBarriers, vkInfo.bufferBarriers, vkInfo.numImageBarriers, vkInfo.imageBarriers);
}
} // namespace CoreGraphics