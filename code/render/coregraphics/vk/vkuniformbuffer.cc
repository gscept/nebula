//------------------------------------------------------------------------------
// vkuniformbuffer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkuniformbuffer.h"
#include "vkrenderdevice.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/config.h"
#include "vkutilities.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkUniformBuffer, 'VKUB', Base::ConstantBufferBase);
//------------------------------------------------------------------------------
/**
*/
VkUniformBuffer::VkUniformBuffer() :
	binding(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkUniformBuffer::~VkUniformBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkUniformBuffer::Setup(const SizeT numBackingBuffers)
{
	ConstantBufferBase::Setup(numBackingBuffers);
	this->byteAlignment = VK_DEVICE_SIZE_CONV(VkRenderDevice::Instance()->deviceProps.limits.minUniformBufferOffsetAlignment);
	//this->byteAlignment = 64;
	uint32_t queues[] = { VkRenderDevice::Instance()->drawQueueFamily, VkRenderDevice::Instance()->computeQueueFamily, VkRenderDevice::Instance()->transferQueueFamily };
	this->createInfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,
		this->size * numBackingBuffers,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
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
	res = vkMapMemory(VkRenderDevice::dev, this->mem, 0, this->size, 0, &this->buffer);
	n_assert(res == VK_SUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void
VkUniformBuffer::SetupFromBlockInShader(const Ptr<CoreGraphics::ShaderState>& shaderState, const Util::String& blockName, const SizeT numBackingBuffers)
{
	ConstantBufferBase::SetupFromBlockInShader(shaderState, blockName, numBackingBuffers);
	const Ptr<CoreGraphics::Shader>& shader = shaderState->GetShader();
	AnyFX::VkVarblock* varblock = static_cast<AnyFX::VkVarblock*>(shader->GetVkEffect()->GetVarblock(blockName.AsCharPtr()));

	// setup buffer from other buffer
	this->binding = varblock->binding;
	this->size = varblock->alignedSize;
	this->Setup(numBackingBuffers);

	// set our buffer to be non-expandable
	StretchyBuffer::SetFull();

	// begin synced update, this will cause the uniform buffer to be setup straight from the start
	for (uint i = 0; i < varblock->variables.size(); i++)
	{
		AnyFX::VkVariable* var = static_cast<AnyFX::VkVariable*>(varblock->variables[i]);
		uint32_t offset = varblock->offsetsByName[var->name];

		Ptr<CoreGraphics::ShaderVariable> svar = CoreGraphics::ShaderVariable::Create();
		Ptr<VkUniformBuffer> thisPtr(this);
		svar->Setup(var, shaderState.downcast<VkShaderState>(), VK_NULL_HANDLE);
		svar->BindToUniformBuffer(thisPtr.downcast<CoreGraphics::ConstantBuffer>(), offset, var->byteSize, (int8_t*)var->currentValue);
		this->variables.Append(svar);
		this->variablesByName.Add(var->name.c_str(), svar);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkUniformBuffer::Discard()
{
	vkUnmapMemory(VkRenderDevice::dev, this->mem);
	vkFreeMemory(VkRenderDevice::dev, this->mem, NULL);
	vkDestroyBuffer(VkRenderDevice::dev, this->buf, NULL);
}

//------------------------------------------------------------------------------
/**
*/
void
VkUniformBuffer::UpdateAsync(void* data, uint offset, uint size)
{
	n_assert(size + offset <= this->size);
	byte* buf = (byte*)this->buffer + offset + this->baseOffset;
	memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
void
VkUniformBuffer::UpdateArrayAsync(void* data, uint offset, uint size, uint count)
{
	n_assert(size + offset <= this->size);
	byte* buf = (byte*)this->buffer + offset + this->baseOffset;
	memcpy(buf, data, size * count);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkUniformBuffer::Grow(SizeT oldCapacity, SizeT growBy)
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

//------------------------------------------------------------------------------
/**
*/
void
VkUniformBuffer::Reset()
{
	IndexT i;
	for (i = 0; i < this->usedIndices.Size(); i++)
	{
		this->freeIndices.Append(this->usedIndices[i]);
	}
	this->usedIndices.Clear();
}

} // namespace Vulkan