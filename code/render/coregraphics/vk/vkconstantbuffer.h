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
#include "coregraphics/constantbuffer.h"
#include "coregraphics/stretchybuffer.h"
#include <lowlevel/afxapi.h>
namespace Vulkan
{

struct VkConstantBufferRuntimeInfo
{
	IndexT binding;
	VkBuffer buf;
};

struct VkConstantBufferSetupInfo
{
	VkDevice dev;
	VkBufferCreateInfo info;
	VkDeviceMemory mem;
	SizeT size;
	SizeT stride;
	SizeT numBuffers;
	SizeT grow;
	AnyFX::VarblockBase* reflection;
};

struct VkConstantBufferMapInfo
{
	void* data;
	SizeT baseOffset;
};

struct ConstantBufferStretchInterface
{
	CoreGraphics::StretchyBuffer<ConstantBufferStretchInterface> resizer;
	CoreGraphics::ConstantBufferId obj;
	SizeT Grow(const SizeT capacity, const SizeT numInstances, SizeT& newCapacity);

	// when we copy from arrays, we might have to reset the pointer...
	void operator=(const ConstantBufferStretchInterface& rhs)
	{
		this->resizer = rhs.resizer;
		this->obj = rhs.obj;
		this->resizer.SetTarget(this);
	}
};

typedef Ids::IdAllocator<
	VkConstantBufferRuntimeInfo,
	VkConstantBufferSetupInfo,
	VkConstantBufferMapInfo,
	ConstantBufferStretchInterface
> VkConstantBufferAllocator;
extern VkConstantBufferAllocator constantBufferAllocator;

/// get Vulkan backing buffer
VkBuffer ConstantBufferGetVk(const CoreGraphics::ConstantBufferId id);

} // namespace Vulkan