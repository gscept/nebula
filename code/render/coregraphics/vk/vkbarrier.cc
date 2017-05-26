//------------------------------------------------------------------------------
// vkbarrier.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkbarrier.h"
#include "vktypes.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include <tuple>

namespace Vulkan
{

__ImplementClass(Vulkan::VkBarrier, 'VKBA', Base::BarrierBase);
//------------------------------------------------------------------------------
/**
*/
VkBarrier::VkBarrier()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkBarrier::~VkBarrier()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkBarrier::Setup()
{
	// setup base
	BarrierBase::Setup();

	// get flags
	this->vkSrcFlags = VkTypes::AsVkPipelineFlags(this->leftDependency);
	this->vkDstFlags = VkTypes::AsVkPipelineFlags(this->rightDependency);

	switch (this->domain)
	{
	case BarrierBase::Domain::Pass: this->vkDep = VK_DEPENDENCY_BY_REGION_BIT;
	}

	// hmm, a memory barrier is truly a memory block, not a resource!
	this->vkNumMemoryBarriers = 0;
	memset(this->vkMemoryBarriers, 0, sizeof(this->vkMemoryBarriers));

	// clear up buffers
	this->vkNumImageBarriers = this->renderTextures.Size() + this->readWriteTextures.Size();
	memset(this->vkImageBarriers, 0, sizeof(this->vkImageBarriers));
	this->vkNumBufferBarriers = this->readWriteBuffers.Size();
	memset(this->vkBufferBarriers, 0, sizeof(this->vkBufferBarriers));

	uint32_t imageBarrierIndex = 0;
	IndexT i;
	for (i = 0; i < this->renderTextures.Size(); i++)
	{
		// to be entirely fair, we might not actually want to have to wait for all levels...
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
		this->vkImageBarriers[imageBarrierIndex].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		this->vkImageBarriers[imageBarrierIndex].pNext = nullptr;

		this->vkImageBarriers[imageBarrierIndex].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<0>(this->renderTexturesAccess[i]));
		this->vkImageBarriers[imageBarrierIndex].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(this->renderTexturesAccess[i]));
		this->vkImageBarriers[imageBarrierIndex].subresourceRange = range;
		this->vkImageBarriers[imageBarrierIndex].image = this->renderTextures[i]->GetVkImage();
		this->vkImageBarriers[imageBarrierIndex].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		this->vkImageBarriers[imageBarrierIndex].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		this->vkImageBarriers[imageBarrierIndex].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		this->vkImageBarriers[imageBarrierIndex].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrierIndex++;
	}

	for (i = 0; i < this->readWriteTextures.Size(); i++)
	{
		// to be entirely fair, we might not actually want to have to wait for all levels...
		VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
		this->vkImageBarriers[imageBarrierIndex].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		this->vkImageBarriers[imageBarrierIndex].pNext = nullptr;

		this->vkImageBarriers[imageBarrierIndex].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<0>(this->readWriteTexturesAccess[i]));
		this->vkImageBarriers[imageBarrierIndex].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(this->readWriteTexturesAccess[i]));
		this->vkImageBarriers[imageBarrierIndex].subresourceRange = range;
		this->vkImageBarriers[imageBarrierIndex].image = this->readWriteTextures[i]->GetVkImage();		
		this->vkImageBarriers[imageBarrierIndex].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		this->vkImageBarriers[imageBarrierIndex].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		this->vkImageBarriers[imageBarrierIndex].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		this->vkImageBarriers[imageBarrierIndex].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrierIndex++;
	}

	for (i = 0; i < this->readWriteBuffers.Size(); i++)
	{
		this->vkBufferBarriers[imageBarrierIndex].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		this->vkBufferBarriers[imageBarrierIndex].pNext = nullptr;

		this->vkBufferBarriers[imageBarrierIndex].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<0>(this->readWriteBuffersAccess[i]));
		this->vkBufferBarriers[imageBarrierIndex].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(this->readWriteBuffersAccess[i]));
		this->vkBufferBarriers[imageBarrierIndex].buffer = this->readWriteBuffers[i]->GetVkBuffer();
		this->vkBufferBarriers[imageBarrierIndex].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		this->vkBufferBarriers[imageBarrierIndex].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrierIndex++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkBarrier::Discard()
{

}

} // namespace Vulkan