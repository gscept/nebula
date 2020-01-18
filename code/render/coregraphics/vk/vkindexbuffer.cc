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
	return iboAllocator.Get<1>(id.id24).buf;
}

//------------------------------------------------------------------------------
/**
*/
VkDeviceMemory 
IndexBufferGetVkMemory(const CoreGraphics::IndexBufferId id)
{
	return iboAllocator.Get<0>(id.id24).mem;
}

//------------------------------------------------------------------------------
/**
*/
VkIndexType
IndexBufferGetVkType(const CoreGraphics::IndexBufferId id)
{
	return iboAllocator.Get<1>(id.id24).type == CoreGraphics::IndexType::Index16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
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
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.Get<0>(id);
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.Get<1>(id);
	uint32_t& mapCount = iboAllocator.Get<2>(id);

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

	// allocate a device memory backing for this
	uint32_t alignedSize;
	uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	flags |= info.sync == CoreGraphics::GpuBufferTypes::SyncingAutomatic ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	if (info.data != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(loadInfo.dev, loadInfo.mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(info.dataSize <= (int32_t)alignedSize);
		memcpy(data, info.data, info.dataSize);
		vkUnmapMemory(loadInfo.dev, loadInfo.mem);
	}

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access, info.sync };
	loadInfo.indexCount = info.numIndices;
	runtimeInfo.type = info.type;
	mapCount = 0;

	IndexBufferId ret;
	ret.id24 = id;
	ret.id8 = IndexBufferIdType;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.AsString());
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
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.Get<0>(id);
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.Get<1>(id);
	uint32_t& mapCount = iboAllocator.Get<2>(id);

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

	// allocate a device memory backing for this
	uint32_t alignedSize;
	uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	flags |= info.sync == CoreGraphics::GpuBufferTypes::SyncingAutomatic ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access, info.sync };
	loadInfo.indexCount = info.size;
	runtimeInfo.type = info.type;
	mapCount = 0;

	IndexBufferId ret;
	ret.id24 = id;
	ret.id8 = IndexBufferIdType;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.AsString());
#endif

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyIndexBuffer(const IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.Get<0>(id.id24);
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.Get<1>(id.id24);
	uint32_t& mapCount = iboAllocator.Get<2>(id.id24);

	n_assert(mapCount == 0);
	vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
	vkDestroyBuffer(loadInfo.dev, runtimeInfo.buf, nullptr);
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
	void* buf;
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.Get<0>(id.id24);
	uint32_t& mapCount = iboAllocator.Get<2>(id.id24);
	VkResult res = vkMapMemory(loadInfo.dev, loadInfo.mem, 0, VK_WHOLE_SIZE, 0, &buf);
	n_assert(res == VK_SUCCESS);
	mapCount++;
	return buf;
}

//------------------------------------------------------------------------------
/**
*/
void
IndexBufferUnmap(const IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.Get<0>(id.id24);
	uint32_t& mapCount = iboAllocator.Get<2>(id.id24);
	vkUnmapMemory(loadInfo.dev, loadInfo.mem);
	mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::IndexType::Code
IndexBufferGetType(const IndexBufferId id)
{
	VkIndexBufferRuntimeInfo& runtimeInfo = iboAllocator.Get<1>(id.id24);
	return runtimeInfo.type;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
IndexBufferGetNumIndices(const IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = iboAllocator.Get<0>(id.id24);
	return loadInfo.indexCount;
}

} // namespace CoreGraphics