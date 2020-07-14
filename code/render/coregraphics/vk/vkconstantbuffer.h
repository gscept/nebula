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
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/constantbuffer.h"
#include "ids/idallocator.h"
#include "vkmemory.h"
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
	CoreGraphics::Alloc mem;
	SizeT size;
	CoreGraphics::BufferUpdateMode mode;
};

struct VkConstantBufferMapInfo
{
	void* data;
	SizeT baseOffset;
};

enum
{
	RuntimeInfo,
	SetupInfo,
	MapInfo
};

typedef Ids::IdAllocator<
	VkConstantBufferRuntimeInfo,
	VkConstantBufferSetupInfo,
	VkConstantBufferMapInfo
> VkConstantBufferAllocator;
extern VkConstantBufferAllocator constantBufferAllocator;

/// get vulkan buffer buffer
VkBuffer ConstantBufferGetVk(const CoreGraphics::ConstantBufferId id);
/// get vulkan buffer memory
VkDeviceMemory ConstantBufferGetVkMemory(const CoreGraphics::ConstantBufferId id);

} // namespace Vulkan