//------------------------------------------------------------------------------
// vkshaderrwbuffer.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderrwbuffer.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/config.h"
#include "vkutilities.h"
#include "vkmemory.h"

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

	VkBufferUsageFlags usageFlags = 0;
	if (info.mode == HostWriteable || info.mode == HostMapped)
		usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	else if (info.mode == DeviceLocal)
		usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	setupInfo.info =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		(VkDeviceSize)(size),
		usageFlags,
		VK_SHARING_MODE_CONCURRENT,
		(uint32_t)queues.Size(),
		queues.KeysAsArray().Begin()
	};
	VkResult res = vkCreateBuffer(setupInfo.dev, &setupInfo.info, nullptr, &runtimeInfo.buf);
	n_assert(res == VK_SUCCESS);

	CoreGraphics::MemoryPoolType pool = CoreGraphics::BufferMemory_Local;
	if (info.mode == HostWriteable)
		pool = CoreGraphics::BufferMemory_Dynamic;
	else if (info.mode == HostMapped)
		pool = CoreGraphics::BufferMemory_Mapped;
	else if (info.mode == DeviceLocal)
		pool = CoreGraphics::BufferMemory_Local;

	// allocate and bind memory
	CoreGraphics::Alloc alloc = AllocateMemory(setupInfo.dev, runtimeInfo.buf, pool);
	res = vkBindBufferMemory(setupInfo.dev, runtimeInfo.buf, alloc.mem, alloc.offset);
	n_assert(res == VK_SUCCESS);

	// size and stride for a single buffer are equal
	setupInfo.size = alloc.size;
	setupInfo.mem = alloc;

	ShaderRWBufferId ret;
	ret.id24 = id;
	ret.id8 = ShaderRWBufferIdType;

	if (info.mode == HostWriteable || info.mode == HostMapped)
	{
		// map memory so we can use it later
		mapInfo.data = (char*)GetMappedMemory(alloc);
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

	Vulkan::DelayedFreeMemory(setupInfo.mem);
	Vulkan::DelayedDeleteBuffer(runtimeInfo.buf);

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

//------------------------------------------------------------------------------
/**
*/
void 
ShaderRWBufferFlush(const ShaderRWBufferId id)
{
	VkShaderRWBufferLoadInfo& setupInfo = shaderRWBufferAllocator.Get<0>(id.id24);
	Flush(setupInfo.dev, setupInfo.mem);
}

} // namespace CoreGraphics