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
	return setup.mem;
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
	VkConstantBufferPool& pool = constantBufferAllocator.Get<AllocPool>(id);

	VkDevice dev = Vulkan::GetCurrentDevice();
	VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();

	setup.binding = info.binding;
	setup.dev = dev;
	setup.mode = info.mode;
	setup.grow = 16;
	SizeT size = info.size;

	VkBufferUsageFlags usageFlags = 0;
	if (info.mode == HostWriteable)
		usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	else if (info.mode == DeviceWriteable)
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

	uint32_t alignedSize;
	VkMemoryPropertyFlagBits flags = VkMemoryPropertyFlagBits(0);
	if (info.mode == HostWriteable)
		flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	else if (info.mode == DeviceWriteable)
		flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	// allocate memory
	VkUtilities::AllocateBufferMemory(setup.dev, runtime.buf, setup.mem, flags, alignedSize);

	// bind to buffer
	res = vkBindBufferMemory(setup.dev, runtime.buf, setup.mem, 0);
	n_assert(res == VK_SUCCESS);

	// size and stride for a single buffer are equal
	setup.size = alignedSize;
	setup.stride = alignedSize;
	pool.capacity = setup.size;

	map.data = nullptr;
	if (info.mode == HostWriteable)
	{
		// map memory so we can use it later, if we are using coherently mapping
		res = vkMapMemory(setup.dev, setup.mem, 0, setup.size, 0, &map.data);
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

	// unmap memory, so we can free it
	if (map.data != nullptr)
		vkUnmapMemory(setup.dev, setup.mem);

	Vulkan::DelayedDeleteMemory(setup.mem);
	Vulkan::DelayedDeleteBuffer(runtime.buf);
	//vkFreeMemory(setup.dev, setup.mem, nullptr);
	//vkDestroyBuffer(setup.dev, runtime.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
bool
ConstantBufferAllocateInstance(const ConstantBufferId id, uint& offset, uint& slice)
{
	VkConstantBufferSetupInfo& setupInfo = constantBufferAllocator.Get<SetupInfo>(id.id24);
	bool needsRebind = false;
	const ConstantBufferAllocId alloc = ConstantBufferAllocate(id, setupInfo.stride, needsRebind);
	offset = alloc.offset;
	slice = offset / setupInfo.stride;
	return needsRebind;
}

//------------------------------------------------------------------------------
/**
*/
void
ConstantBufferFreeInstance(ConstantBufferId id, uint slice)
{
	VkConstantBufferSetupInfo& setupInfo = constantBufferAllocator.Get<SetupInfo>(id.id24);
	ConstantBufferFree(id, ConstantBufferAllocId((Ids::Id32)(setupInfo.stride * slice), (Ids::Id32)setupInfo.stride));
}

//------------------------------------------------------------------------------
/**
*/
ConstantBufferAllocId 
ConstantBufferAllocate(const ConstantBufferId id, const SizeT size, bool& needsRebind)
{
	VkConstantBufferPool& pool = constantBufferAllocator.Get<AllocPool>(id.id24);
	needsRebind = false;

	// find free alloc, and reuse it
	IndexT i;
	for (i = 0; i < pool.freeAllocs.Size(); i++)
	{
		const ConstantBufferAllocId& alloc = pool.freeAllocs[i];
		if ((SizeT)alloc.size >= size)
			return alloc;
	}

	// if we have enough memory, just create a new id with the offset to the memory
	ConstantBufferAllocId ret;
	ret.size = size;
	if (pool.size + size < pool.capacity)
	{
		ret.offset = pool.size;
		pool.size += size;
		return ret;
	}
	else
	{
		// we need to grow the buffer!
		VkConstantBufferRuntimeInfo& runtimeInfo = constantBufferAllocator.Get<RuntimeInfo>(id.id24);
		VkConstantBufferSetupInfo& setupInfo = constantBufferAllocator.Get<SetupInfo>(id.id24);
		VkConstantBufferMapInfo& mapInfo = constantBufferAllocator.Get<MapInfo>(id.id24);
#if NEBULA_DEBUG
		n_assert(id.id8 == ConstantBufferIdType);
#endif
		// new capacity is the old one, plus the number of elements we wish to allocate, although never allocate fewer than grow
		SizeT increment = (pool.capacity / setupInfo.stride) >> 1; // increment should hold the number of elements
		increment = Math::n_iclamp(increment, setupInfo.grow, 65535);
		pool.capacity = pool.capacity + increment * setupInfo.stride; // so we want to calculate the new size as a multiple of the buffer block

		// create new buffer
		const Util::Set<uint32_t>& queues = Vulkan::GetQueueFamilies();
		setupInfo.info.pQueueFamilyIndices = queues.KeysAsArray().Begin();
		setupInfo.info.queueFamilyIndexCount = (uint32_t)queues.Size();
		setupInfo.info.size = pool.capacity;

		VkBuffer newBuf;
		VkResult res = vkCreateBuffer(setupInfo.dev, &setupInfo.info, NULL, &newBuf);
		n_assert(res == VK_SUCCESS);

		// allocate new instance memory, alignedSize is the aligned size of a single buffer
		VkDeviceMemory newMem;
		uint32_t alignedSize;
		VkMemoryPropertyFlagBits flags = VkMemoryPropertyFlagBits(0);
		if (setupInfo.mode == HostWriteable)
			flags = VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		else if (setupInfo.mode == DeviceWriteable)
			flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		VkUtilities::AllocateBufferMemory(setupInfo.dev, newBuf, newMem, flags, alignedSize);

		// bind to buffer, this is the reason why we must destroy and create the buffer again
		res = vkBindBufferMemory(setupInfo.dev, newBuf, newMem, 0);
		n_assert(res == VK_SUCCESS);

		// copy old to new, old memory is already mapped
		void* dstData;

		// map new memory with new capacity, avoids a second map
		res = vkMapMemory(setupInfo.dev, newMem, 0, alignedSize, 0, &dstData);
		n_assert(res == VK_SUCCESS);
		memcpy(dstData, mapInfo.data, setupInfo.size);
		vkUnmapMemory(setupInfo.dev, setupInfo.mem);

		// clean up old data	
		vkDestroyBuffer(setupInfo.dev, runtimeInfo.buf, nullptr);
		vkFreeMemory(setupInfo.dev, setupInfo.mem, nullptr);

		// replace old device memory and size
		setupInfo.size = alignedSize;
		runtimeInfo.buf = newBuf;
		setupInfo.mem = newMem;
		mapInfo.data = dstData;
		pool.capacity = setupInfo.size;

		// update pool and get the offset
		ret.offset = pool.size;
		pool.size += size;

		// we allocated a new buffer, so we have to rebind it to whatever resource table we use it in!
		needsRebind = true;
	}

	// if the loop fails, we have to allocate more memory
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void 
ConstantBufferFree(const ConstantBufferId id, const ConstantBufferAllocId alloc)
{
	VkConstantBufferPool& pool = constantBufferAllocator.Get<AllocPool>(id.id24);
	pool.freeAllocs.Append(alloc);
}

//------------------------------------------------------------------------------
/**
*/
IndexT
ConstantBufferGetSlot(const ConstantBufferId id)
{
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);
	return setup.binding;
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
ConstantBufferUpdateInstance(const ConstantBufferId id, const void* data, const uint size, const uint instance, ConstantBinding bind)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);

#if NEBULA_DEBUG
	n_assert(size + bind + setup.stride * instance <= (uint)setup.size);
#endif
	byte* buf = (byte*)map.data + bind + setup.stride * instance;
	memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
void 
ConstantBufferUpdateArrayInstance(const ConstantBufferId id, const void* data, const uint size, const uint count, const uint instance, ConstantBinding bind)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);
	VkConstantBufferSetupInfo& setup = constantBufferAllocator.Get<SetupInfo>(id.id24);

#if NEBULA_DEBUG
	n_assert(size + bind + setup.stride * instance <= (uint)setup.size);
#endif
	byte* buf = (byte*)map.data + bind + setup.stride * instance;
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
ConstantBufferSetBaseOffset(const ConstantBufferId id, const uint offset)
{
	VkConstantBufferMapInfo& map = constantBufferAllocator.Get<MapInfo>(id.id24);
	map.baseOffset = offset;
}

} // namespace CoreGraphics
