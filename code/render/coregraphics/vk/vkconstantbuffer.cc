//------------------------------------------------------------------------------
// vkconstantbuffer.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkgraphicsdevice.h"
#include "vkconstantbuffer.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/config.h"
#include "vkutilities.h"
#include "coregraphics/shaderpool.h"
#include "lowlevel/vk/vkvarblock.h"
#include "vkmemory.h"

namespace Vulkan
{

VkConstantBufferAllocator constantBufferAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
VkBuffer
ConstantBufferGetVk(const CoreGraphics::ConstantBufferId id)
{
	VkConstantBufferRuntimeInfo& runtime = constantBufferAllocator.Get<RuntimeInfo>(id.id24);
	return runtime.buf;
}

//------------------------------------------------------------------------------
/**
*/
VkDeviceMemory 
ConstantBufferGetVkMemory(const CoreGraphics::ConstantBufferId id)
{
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);
	return setup.mem.mem;
}

} // namespace Vulkan


namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
const ConstantBufferId
CreateConstantBuffer(const ConstantBufferCreateInfo& info)
{
	Ids::Id32 id = constantBufferAllocator.Alloc();
	VkConstantBufferRuntimeInfo& runtime = constantBufferAllocator.Get<RuntimeInfo>(id);
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id);
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id);

	VkDevice dev = Vulkan::GetCurrentDevice();
	VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();

	setup.dev = dev;
	setup.mode = info.mode;
	SizeT size = info.size;

	VkBufferUsageFlags usageFlags = 0;
	if (info.mode == HostWriteable || info.mode == HostMapped)
		usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	else if (info.mode == DeviceLocal)
		usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	const Util::FixedArray<uint32_t> queues = { Vulkan::GetQueueFamily(GraphicsQueueType), Vulkan::GetQueueFamily(ComputeQueueType) };
	setup.info =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,
		(VkDeviceSize)size,
		usageFlags,
		VK_SHARING_MODE_EXCLUSIVE,
		(uint32_t)queues.Size(),
		queues.Begin()
	};
	VkResult res = vkCreateBuffer(setup.dev, &setup.info, NULL, &runtime.buf);
	n_assert(res == VK_SUCCESS);

	map.data = nullptr;

	CoreGraphics::MemoryPoolType pool = CoreGraphics::MemoryPool_DeviceLocal;
	if (info.mode == HostWriteable)
		pool = CoreGraphics::MemoryPool_ManualFlush;
	else if (info.mode == HostMapped)
		pool = CoreGraphics::MemoryPool_HostCoherent;
	else if (info.mode == DeviceLocal)
		pool = CoreGraphics::MemoryPool_DeviceLocal;

	// allocate and bind memory
	CoreGraphics::Alloc alloc = AllocateMemory(dev, runtime.buf, pool);
	setup.mem = alloc;
	setup.size = alloc.size;

	// bind to buffer
	res = vkBindBufferMemory(setup.dev, runtime.buf, alloc.mem, alloc.offset);
	n_assert(res == VK_SUCCESS);

	if (info.mode == HostWriteable || info.mode == HostMapped)
	{
		// map memory so we can use it later, if we are using coherently mapping
		map.data = (char*)GetMappedMemory(alloc);
		//res = vkMapMemory(dev, mem, 0, size, 0, &map.data);
		n_assert(res == VK_SUCCESS);
	}

	ConstantBufferId ret;
	ret.id24 = id;
	ret.id8 = ConstantBufferIdType;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.Value());
#endif

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyConstantBuffer(const ConstantBufferId id)
{
	VkConstantBufferRuntimeInfo& runtime = constantBufferAllocator.Get<RuntimeInfo>(id.id24);
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);

	Vulkan::DelayedFreeMemory(setup.mem);
	Vulkan::DelayedDeleteBuffer(runtime.buf);
}

//------------------------------------------------------------------------------
/**
*/
void 
ConstantBufferUpdate(const ConstantBufferId id, const void* data, const uint size, ConstantBinding bind)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);

#if NEBULA_DEBUG
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);
	n_assert(size + bind <= (uint)setup.size);
#endif
	byte* buf = (byte*)map.data + bind;
	memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
void 
ConstantBufferUpdateArray(const ConstantBufferId id, const void* data, const uint size, const uint count, ConstantBinding bind)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);

#if NEBULA_DEBUG
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);
	n_assert(size + bind <= (uint)setup.size);
#endif
	byte* buf = (byte*)map.data + bind;
	memcpy(buf, data, size * count);
}

//------------------------------------------------------------------------------
/**
*/
void 
ConstantBufferUpdate(const ConstantBufferId id, const ConstantBufferAllocId alloc, const void* data, const uint size, ConstantBinding bind)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);

#if NEBULA_DEBUG
	n_assert(size >= alloc.size);
	n_assert(size + bind + alloc.offset <= (uint)setup.size);
#endif
	byte* buf = (byte*)map.data + bind + alloc.offset;
	memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
void 
ConstantBufferFlush(const ConstantBufferId id)
{
	VkConstantBufferSetupInfo& loadInfo = constantBufferAllocator.Get<SetupInfo>(id.id24);
	Flush(loadInfo.dev, loadInfo.mem);
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferSetBaseOffset(const ConstantBufferId id, const uint offset)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);
	map.baseOffset = offset;
}

} // namespace CoreGraphics
