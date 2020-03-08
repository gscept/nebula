//------------------------------------------------------------------------------
// vkshaderrwbuffer.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderrwbuffer.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/config.h"
#include "vkutilities.h"

namespace Vulkan
{

ShaderRWBufferAllocator shaderRWBufferAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkBuffer
ShaderRWBufferGetVkBuffer(const CoreGraphics::ShaderRWBufferId id)
{
	return shaderRWBufferAllocator.Get<1>(id.id24).buf;
}

} // namespace Vulkan

namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
const ShaderRWBufferId
CreateShaderRWBuffer(const ShaderRWBufferCreateInfo& info)
{
	Ids::Id32 id = shaderRWBufferAllocator.Alloc();

	VkShaderRWBufferLoadInfo& setupInfo = shaderRWBufferAllocator.Get<0>(id);
	VkShaderRWBufferRuntimeInfo& runtimeInfo = shaderRWBufferAllocator.Get<1>(id);
	VkShaderRWBufferMapInfo& mapInfo = shaderRWBufferAllocator.Get<2>(id);

	VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();
	setupInfo.dev = Vulkan::GetCurrentDevice();
	SizeT size = info.size;

	const Util::Set<uint32_t>& queues = Vulkan::GetQueueFamilies();
	setupInfo.info =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		(VkDeviceSize)(size),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_SHARING_MODE_CONCURRENT,
		(uint32_t)queues.Size(),
		queues.KeysAsArray().Begin()
	};
	VkResult res = vkCreateBuffer(setupInfo.dev, &setupInfo.info, nullptr, &runtimeInfo.buf);
	n_assert(res == VK_SUCCESS);

	uint32_t alignedSize;
	VkMemoryPropertyFlagBits flags = VkMemoryPropertyFlagBits(0);
	if (info.mode == HostWriteable)
		flags = VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	else if (info.mode == DeviceWriteable)
		flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkUtilities::AllocateBufferMemory(setupInfo.dev, runtimeInfo.buf, setupInfo.mem, flags, alignedSize);

	// bind to buffer
	res = vkBindBufferMemory(setupInfo.dev, runtimeInfo.buf, setupInfo.mem, 0);
	n_assert(res == VK_SUCCESS);

	// size and stride for a single buffer are equal
	setupInfo.size = alignedSize;
	setupInfo.stride = alignedSize / setupInfo.numBuffers;

	ShaderRWBufferId ret;
	ret.id24 = id;
	ret.id8 = ShaderRWBufferIdType;

	if (info.mode == HostWriteable)
	{
		// map memory so we can use it later
		res = vkMapMemory(setupInfo.dev, setupInfo.mem, 0, alignedSize, 0, &mapInfo.data);
		n_assert(res == VK_SUCCESS);
	}
	

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.Value());
#endif

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyShaderRWBuffer(const ShaderRWBufferId id)
{
	VkShaderRWBufferLoadInfo& setupInfo = shaderRWBufferAllocator.Get<0>(id.id24);
	VkShaderRWBufferRuntimeInfo& runtimeInfo = shaderRWBufferAllocator.Get<1>(id.id24);

	vkUnmapMemory(setupInfo.dev, setupInfo.mem);

	Vulkan::DelayedDeleteBuffer(runtimeInfo.buf);
	Vulkan::DelayedDeleteMemory(setupInfo.mem);
	//vkDestroyBuffer(setupInfo.dev, runtimeInfo.buf, nullptr);
	//vkFreeMemory(setupInfo.dev, setupInfo.mem, nullptr);

	shaderRWBufferAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderRWBufferUpdate(const ShaderRWBufferId id, void* data, SizeT bytes)
{
	VkShaderRWBufferMapInfo& mapInfo = shaderRWBufferAllocator.Get<2>(id.id24);
#if NEBULA_DEBUG
	VkShaderRWBufferLoadInfo& setupInfo = shaderRWBufferAllocator.Get<0>(id.id24);
	n_assert(setupInfo.size >= bytes);
#endif
	memcpy(mapInfo.data, data, bytes);
}

} // namespace CoreGraphics