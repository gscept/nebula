#pragma once
//------------------------------------------------------------------------------
/**
	Implements a uniform buffer used for shader uniforms in Vulkan.

	Allocates memory storage by expanding size using AllocateInstance, and returns
	a free allocation using FreeInstance. Memory is never destroyed, only grown, however
	discard will properly destroy the uniform buffer and release its memory.

	In order to use instances with SetupFromBlockInShader, where variables are fetched from 
	the uniform buffer, use SetActiveInstance to automatically have the variables update data
	at that instances offset.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/constantbufferbase.h"
namespace Vulkan
{
class VkUniformBuffer : public Base::ConstantBufferBase
{
	__DeclareClass(VkUniformBuffer);
public:
	/// constructor
	VkUniformBuffer();
	/// destructor
	virtual ~VkUniformBuffer();

	/// setup buffer
	void Setup(const SizeT numBackingBuffers = 1);
	/// bind variables in a block with a name in a shader to this buffer (only do this on system managed blocks)
	void SetupFromBlockInShader(const Ptr<CoreGraphics::ShaderState>& shader, const Util::String& blockName, const SizeT numBackingBuffers = 1);
	/// discard buffer
	void Discard();

	/// clears used list without deallocating memory
	void Reset();
	/// get binding, valid only if SetupFromBlockInShader is used
	const IndexT GetBinding() const;

	/// get buffer
	const VkBuffer& GetVkBuffer() const;
	/// get memory
	const VkDeviceMemory& GetVkMemory() const;
private:

	/// update buffer asynchronously, depending on implementation, this might overwrite data before used
	virtual void UpdateAsync(void* data, uint offset, uint size);
	/// update segment of buffer as array, depending on implementation, this might overwrite data before used
	virtual void UpdateArrayAsync(void* data, uint offset, uint size, uint count);

	/// grow uniform buffer, returns new aligned size
	uint32_t Grow(SizeT oldCapacity, SizeT growBy);

	IndexT binding;
	VkBufferCreateInfo createInfo;
	VkDeviceMemory mem;
	VkBuffer buf;
};

//------------------------------------------------------------------------------
/**
*/
inline const VkBuffer&
VkUniformBuffer::GetVkBuffer() const
{
	return this->buf;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkDeviceMemory&
VkUniformBuffer::GetVkMemory() const
{
	return this->mem;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT
VkUniformBuffer::GetBinding() const
{
	return this->binding;
}

} // namespace Vulkan