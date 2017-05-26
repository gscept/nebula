#pragma once
//------------------------------------------------------------------------------
/**
	Implements a read/write buffer used within shaders, in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/shaderreadwritebufferbase.h"
namespace Vulkan
{
class VkShaderStorageBuffer : public Base::ShaderReadWriteBufferBase
{
	__DeclareClass(VkShaderStorageBuffer);
public:
	/// constructor
	VkShaderStorageBuffer();
	/// destructor
	virtual ~VkShaderStorageBuffer();

	/// setup buffer
	void Setup(const SizeT numBackingBuffers = DefaultNumBackingBuffers);
	/// discard buffer
	void Discard();

	/// get buffer
	const VkBuffer& GetVkBuffer() const;
	/// get memory
	const VkDeviceMemory& GetVkMemory() const;
private:

	/// grow uniform buffer, returns new aligned size
	uint32_t Grow(SizeT oldCapacity, SizeT growBy);

	void* buffer;
	VkBufferCreateInfo createInfo;
	VkBuffer buf;
	VkDeviceMemory mem;
};


//------------------------------------------------------------------------------
/**
*/
inline const VkBuffer&
VkShaderStorageBuffer::GetVkBuffer() const
{
	return this->buf;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkDeviceMemory&
VkShaderStorageBuffer::GetVkMemory() const
{
	return this->mem;
}
} // namespace Vulkan