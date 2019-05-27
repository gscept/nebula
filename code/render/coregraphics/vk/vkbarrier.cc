//------------------------------------------------------------------------------
// vkbarrier.cc
// (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkbarrier.h"
#include "coregraphics/config.h"
#include "vkcmdbuffer.h"
#include "vktypes.h"
#include "vkrendertexture.h"
#include "vkshaderrwtexture.h"
#include "vkshaderrwbuffer.h"
#include "coregraphics/graphicsdevice.h"

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

	vkInfo.numImageBarriers = 0;
	vkInfo.numBufferBarriers = 0;
	vkInfo.numMemoryBarriers = 0;
	vkInfo.srcFlags = VkTypes::AsVkPipelineFlags(info.leftDependency);
	vkInfo.dstFlags = VkTypes::AsVkPipelineFlags(info.rightDependency);

	if (info.domain == BarrierDomain::Pass)
		vkInfo.dep = VK_DEPENDENCY_BY_REGION_BIT;

	n_assert(info.renderTextures.Size() < MaxNumBarriers);
	n_assert(info.shaderRWTextures.Size() < MaxNumBarriers);
	n_assert(info.shaderRWBuffers.Size() < MaxNumBarriers);

	IndexT i;
	for (i = 0; i < info.renderTextures.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<4>(info.renderTextures[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<5>(info.renderTextures[i]));

		const ImageSubresourceInfo& subres = std::get<1>(info.renderTextures[i]);
		vkInfo.imageBarriers[i].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkInfo.imageBarriers[i].subresourceRange.baseMipLevel = subres.mip;
		vkInfo.imageBarriers[i].subresourceRange.levelCount = subres.mipCount;
		vkInfo.imageBarriers[i].subresourceRange.baseArrayLayer = subres.layer;
		vkInfo.imageBarriers[i].subresourceRange.layerCount = subres.layerCount;
		vkInfo.imageBarriers[i].image = RenderTextureGetVkImage(std::get<0>(info.renderTextures[i]));
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VkTypes::AsVkImageLayout(std::get<2>(info.renderTextures[i]));
		vkInfo.imageBarriers[i].newLayout = VkTypes::AsVkImageLayout(std::get<3>(info.renderTextures[i]));
		vkInfo.numImageBarriers++;

		rts.Append(std::get<0>(info.renderTextures[i]));
	}

	// make sure we have room...
	n_assert(info.shaderRWTextures.Size() < MaxNumBarriers - i);
	for (; i < info.shaderRWTextures.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<4>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<5>(info.shaderRWTextures[i]));

		const ImageSubresourceInfo& subres = std::get<1>(info.shaderRWTextures[i]);
		vkInfo.imageBarriers[i].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkInfo.imageBarriers[i].subresourceRange.baseMipLevel = subres.mip;
		vkInfo.imageBarriers[i].subresourceRange.levelCount = subres.mipCount;
		vkInfo.imageBarriers[i].subresourceRange.baseArrayLayer = subres.layer;
		vkInfo.imageBarriers[i].subresourceRange.layerCount = subres.layerCount;
		vkInfo.imageBarriers[i].image = ShaderRWTextureGetVkImage(std::get<0>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VkTypes::AsVkImageLayout(std::get<2>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].newLayout = VkTypes::AsVkImageLayout(std::get<3>(info.shaderRWTextures[i]));
		vkInfo.numImageBarriers++;

		rws.Append(std::get<0>(info.shaderRWTextures[i]));
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
	barrierAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierInsert(const BarrierId id, const CoreGraphicsQueueType queue)
{
	CoreGraphics::InsertBarrier(id, queue);
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

} // namespace CoreGraphics