//------------------------------------------------------------------------------
// vkshaderstoragebuffer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshaderstoragebuffer.h"
#include "vkrenderdevice.h"
#include "coregraphics/config.h"
#include "vkutilities.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderStorageBuffer, 'VKSB', Base::ShaderReadWriteBufferBase);
//------------------------------------------------------------------------------
/**
*/
VkShaderStorageBuffer::VkShaderStorageBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkShaderStorageBuffer::~VkShaderStorageBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStorageBuffer::Setup(const SizeT numBackingBuffers)
{
	ShaderReadWriteBufferBase::Setup(numBackingBuffers);
	this->byteAlignment = VK_DEVICE_SIZE_CONV(VkRenderDevice::Instance()->deviceProps.limits.minStorageBufferOffsetAlignment);
	uint32_t queues[] = { VkRenderDevice::Instance()->drawQueueFamily, VkRenderDevice::Instance()->computeQueueFamily, VkRenderDevice::Instance()->transferQueueFamily };
	this->createInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,
		this->size * numBackingBuffers,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_SHARING_MODE_CONCURRENT,
		sizeof(queues) / sizeof(uint32_t),
		queues
	};
	VkResult res = vkCreateBuffer(VkRenderDevice::dev, &this->createInfo, NULL, &this->buf);
	n_assert(res == VK_SUCCESS);

	uint32_t alignedSize;
	VkUtilities::AllocateBufferMemory(this->buf, this->mem, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), alignedSize);

	// bind to buffer
	res = vkBindBufferMemory(VkRenderDevice::dev, this->buf, this->mem, 0);
	n_assert(res == VK_SUCCESS);

	// size and stride for a single buffer are equal
	this->size = alignedSize;
	this->stride = alignedSize / numBackingBuffers;

	// map memory so we can use it later
	res = vkMapMemory(VkRenderDevice::dev, this->mem, 0, alignedSize, 0, &this->buffer);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderStorageBuffer::Discard()
{
	vkUnmapMemory(VkRenderDevice::dev, this->mem);
	vkFreeMemory(VkRenderDevice::dev, this->mem, NULL);
	vkDestroyBuffer(VkRenderDevice::dev, this->buf, NULL);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderStorageBuffer::Grow(SizeT oldCapacity, SizeT growBy)
{
	// new capacity is the old one, plus the number of elements we wish to allocate, although never allocate fewer than grow
	SizeT increment = oldCapacity >> 1;
	increment = Math::n_iclamp(increment, this->grow, 65535);
	n_assert(increment >= growBy);
	SizeT newCapacity = oldCapacity + increment;

	// create new buffer
	uint32_t queues[] = { VkRenderDevice::Instance()->drawQueueFamily, VkRenderDevice::Instance()->computeQueueFamily, VkRenderDevice::Instance()->transferQueueFamily };
	this->createInfo.pQueueFamilyIndices = queues;
	this->createInfo.size = newCapacity * this->stride;

	VkBuffer newBuf;
	VkResult res = vkCreateBuffer(VkRenderDevice::dev, &this->createInfo, NULL, &newBuf);
	n_assert(res == VK_SUCCESS);

	// allocate new instance memory, alignedSize is the aligned size of a single buffer
	VkDeviceMemory newMem;
	uint32_t alignedSize;
	VkUtilities::AllocateBufferMemory(newBuf, newMem, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT), alignedSize);

	// bind to buffer, this is the reason why we must destroy and create the buffer again
	res = vkBindBufferMemory(VkRenderDevice::dev, newBuf, newMem, 0);
	n_assert(res == VK_SUCCESS);

	// copy old to new, old memory is already mapped
	void* dstData;

	// map new memory with new capacity, avoids a second map
	res = vkMapMemory(VkRenderDevice::dev, newMem, 0, alignedSize, 0, &dstData);
	n_assert(res == VK_SUCCESS);
	memcpy(dstData, this->buffer, this->size);
	vkUnmapMemory(VkRenderDevice::dev, this->mem);

	// clean up old data	
	vkDestroyBuffer(VkRenderDevice::dev, this->buf, NULL);
	vkFreeMemory(VkRenderDevice::dev, this->mem, NULL);

	// setup free indices
	StretchyBuffer::SetFree(oldCapacity + growBy, newCapacity - oldCapacity - growBy);

	// replace old device memory and size
	this->size = alignedSize;
	this->buf = newBuf;
	this->mem = newMem;
	this->buffer = dstData;
	return alignedSize;
}

} // namespace Vulkan