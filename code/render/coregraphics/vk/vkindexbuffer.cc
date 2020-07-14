//------------------------------------------------------------------------------
//  vkindexbuffer.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/vk/vkgraphicsdevice.h"
#include "coregraphics/vk/vkutilities.h"
#include "vkindexbuffer.h"
namespace Vulkan
{
VkIndexBufferAllocator iboAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
VkBuffer 
IndexBufferGetVk(const CoreGraphics::IndexBufferId id)
{
	return iboAllocator.GetUnsafe<1>(id.id24).buf;
}

//------------------------------------------------------------------------------
/**
*/
VkDeviceMemory 
IndexBufferGetVkMemory(const CoreGraphics::IndexBufferId id)
{
	return iboAllocator.GetUnsafe<0>(id.id24).mem.mem;
}

//------------------------------------------------------------------------------
/**
*/
VkIndexType
IndexBufferGetVkType(const CoreGraphics::IndexBufferId id)
{
	return iboAllocator.GetUnsafe<1>(id.id24).type == CoreGraphics::IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
}
} // namespace Vulkan


namespace CoreGraphics
{

using namespace Vulkan;
//------------------------------------------------------------------------------
/**
*/
const IndexBufferId
CreateIndexBuffer(const IndexBufferCreateInfo& info)
{
	n_assert(info.type != IndexType::None);
	n_assert(info.numIndices > 0);
	if (CoreGraphics::GpuBufferTypes::UsageImmutable == info.usage)
	{
		n_assert(info.dataSize == (info.numIndices * IndexType::SizeOf(info.type)));
		n_assert(0 != info.data);
		n_assert(0 < info.dataSize);
	}

	Ids::Id32 id = iboAllocator.Alloc();
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.GetUnsafe<0>(id);
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.GetUnsafe<1>(id);
	VkIndexBufferMapInfo& mapInfo = iboAllocator.GetUnsafe<2>(id);

	loadInfo.dev = Vulkan::GetCurrentDevice();
	uint32_t qfamily = Vulkan::GetQueueFamily(GraphicsQueueType);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		(uint32_t)(info.numIndices * IndexType::SizeOf(info.type)),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,						// can only be accessed from the creator queue,
		0,												// number of queues in family
		nullptr											// array of queues belonging to family
	};

	VkResult err = vkCreateBuffer(loadInfo.dev, &bufinfo, NULL, &runtimeInfo.buf);
	n_assert(err == VK_SUCCESS);

	CoreGraphics::MemoryPoolType pool = CoreGraphics::MemoryPool_DeviceLocal;
	if (info.mode == HostWriteable)
		pool = CoreGraphics::MemoryPool_ManualFlush;
	else if (info.mode == HostMapped)
		pool = CoreGraphics::MemoryPool_HostCoherent;
	else if (info.mode == DeviceLocal)
		pool = CoreGraphics::MemoryPool_DeviceLocal;

	// allocate and bind memory
	CoreGraphics::Alloc alloc = AllocateMemory(loadInfo.dev, runtimeInfo.buf, pool);
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, alloc.mem, alloc.offset);
	n_assert(err == VK_SUCCESS);
	loadInfo.mem = alloc;

	if (info.mode == HostWriteable || info.mode == HostMapped)
	{
		// copy contents and flush memory
		char* data = (char*)GetMappedMemory(alloc);
		mapInfo.mappedMemory = data;

		// if we have data, copy the memory to the region
		if (info.data)
		{
			memcpy(data, info.data, info.dataSize);

			// if dynamic memory type, flush the range of data we want to push
			if (pool == CoreGraphics::MemoryPool_ManualFlush)
			{
				VkPhysicalDeviceProperties props = Vulkan::GetCurrentProperties();

				VkMappedMemoryRange range;
				range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.pNext = nullptr;
				range.offset = Math::n_align_down(alloc.offset, props.limits.nonCoherentAtomSize);
				range.size = Math::n_align(alloc.size, props.limits.nonCoherentAtomSize);
				range.memory = alloc.mem;
				VkResult res = vkFlushMappedMemoryRanges(loadInfo.dev, 1, &range);
				n_assert(res == VK_SUCCESS);
			}
		}
	}

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access };
	loadInfo.indexCount = info.numIndices;
	loadInfo.mode = info.mode;
	runtimeInfo.type = info.type;
	mapInfo.mapCount = 0;

	IndexBufferId ret;
	ret.id24 = id;
	ret.id8 = IndexBufferIdType;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.Value());
#endif

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
const IndexBufferId
CreateIndexBuffer(const IndexBufferCreateDirectInfo& info)
{
	n_assert(info.type != IndexType::None);
	n_assert(CoreGraphics::GpuBufferTypes::UsageImmutable != info.usage)

	Ids::Id32 id = iboAllocator.Alloc();
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.GetUnsafe<0>(id);
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.GetUnsafe<1>(id);
	VkIndexBufferMapInfo& mapInfo = iboAllocator.GetUnsafe<2>(id);

	loadInfo.dev = Vulkan::GetCurrentDevice();
	uint32_t qfamily = Vulkan::GetQueueFamily(GraphicsQueueType);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		info.size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,						// can only be accessed from the creator queue,
		0,												// number of queues in family
		nullptr											// array of queues belonging to family
	};

	VkResult err = vkCreateBuffer(loadInfo.dev, &bufinfo, NULL, &runtimeInfo.buf);
	n_assert(err == VK_SUCCESS);

	CoreGraphics::MemoryPoolType pool = CoreGraphics::MemoryPool_DeviceLocal;
	if (info.mode == HostWriteable)
		pool = CoreGraphics::MemoryPool_ManualFlush;
	else if (info.mode == HostMapped)
		pool = CoreGraphics::MemoryPool_HostCoherent;
	else if (info.mode == DeviceLocal)
		pool = CoreGraphics::MemoryPool_DeviceLocal;

	// allocate and bind memory
	CoreGraphics::Alloc alloc = AllocateMemory(loadInfo.dev, runtimeInfo.buf, pool);
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, alloc.mem, alloc.offset);
	n_assert(err == VK_SUCCESS);
	loadInfo.mem = alloc;

	if (info.mode == HostWriteable || info.mode == HostMapped)
	{
		// copy contents and flush memory
		char* data = (char*)GetMappedMemory(alloc);
		mapInfo.mappedMemory = data;
	}

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access };
	loadInfo.indexCount = info.size;
	runtimeInfo.type = info.type;
	mapInfo.mapCount = 0;
	mapInfo.mappedMemory = 0;

	IndexBufferId ret;
	ret.id24 = id;
	ret.id8 = IndexBufferIdType;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.Value());
#endif

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyIndexBuffer(const IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.GetUnsafe<0>(id.id24);
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.GetUnsafe<1>(id.id24);
	VkIndexBufferMapInfo& mapInfo = iboAllocator.GetUnsafe<2>(id.id24);

	n_assert(mapInfo.mapCount == 0);
	Vulkan::DelayedFreeMemory(loadInfo.mem);
	Vulkan::DelayedDeleteBuffer(runtimeInfo.buf);
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferUpdate(const IndexBufferId id, void* data, PtrDiff size, PtrDiff offset)
{
	n_error("Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferLock(const IndexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	n_error("Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferUnlock(const IndexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	n_error("Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void*
IndexBufferMap(const IndexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type)
{
	VkIndexBufferMapInfo& mapInfo = iboAllocator.GetUnsafe<2>(id.id24);
	n_assert_fmt(mapInfo.mappedMemory != nullptr, "Index Buffer must be created as dynamic or mapped to support mapping");
	mapInfo.mapCount++;
	return mapInfo.mappedMemory;
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferUnmap(const IndexBufferId id)
{
	VkIndexBufferMapInfo& mapInfo = iboAllocator.GetUnsafe<2>(id.id24);
	mapInfo.mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
void 
IndexBufferFlush(const IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.GetUnsafe<IndexBuffer_LoadInfo>(id.id24);
	Flush(loadInfo.dev, loadInfo.mem);
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::IndexType::Code
IndexBufferGetType(const IndexBufferId id)
{
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.GetUnsafe<1>(id.id24);
	return runtimeInfo.type;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
IndexBufferGetNumIndices(const IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.GetUnsafe<0>(id.id24);
	return loadInfo.indexCount;
}

} // namespace CoreGraphics