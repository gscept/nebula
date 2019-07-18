//------------------------------------------------------------------------------
//  vkvertexbuffer.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/vk/vkgraphicsdevice.h"
#include "coregraphics/vk/vkutilities.h"
#include "vkvertexbuffer.h"

namespace Vulkan
{
VkVertexBufferAllocator vboAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
VkBuffer 
VertexBufferGetVk(const CoreGraphics::VertexBufferId id)
{
	return vboAllocator.Get<1>(id.id24).buf;
}

//------------------------------------------------------------------------------
/**
*/
VkDeviceMemory 
VertexBufferGetVkMemory(const CoreGraphics::VertexBufferId id)
{
	return vboAllocator.Get<0>(id.id24).mem;
}

} // namespace Vulkan


namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
const VertexBufferId
CreateVertexBuffer(const VertexBufferCreateInfo& info)
{
	n_assert(info.numVerts > 0);
	if (CoreGraphics::GpuBufferTypes::UsageImmutable == info.usage)
	{
		n_assert(0 != info.data);
		n_assert(0 < info.dataSize);
	}

	Ids::Id32 id = vboAllocator.Alloc();
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.Get<0>(id);
	VkVertexBufferRuntimeInfo& runtimeInfo = vboAllocator.Get<1>(id);
	uint32_t& mapCount = vboAllocator.Get<2>(id);

	loadInfo.dev = Vulkan::GetCurrentDevice();

	// create vertex layout
	VertexLayoutCreateInfo vertexLayoutCreateInfo =
	{
		info.comps,
	};
	VertexLayoutId layout = CreateVertexLayout(vertexLayoutCreateInfo);
	SizeT vertexSize = VertexLayoutGetSize(layout);
	uint32_t qfamily = Vulkan::GetQueueFamily(GraphicsQueueType);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		(uint32_t)(vertexSize * info.numVerts),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,						// can only be accessed from the creator queue,
		0,												// number of queues in family
		nullptr											// array of queues belonging to family
	};

	VkResult err = vkCreateBuffer(loadInfo.dev, &bufinfo, NULL, &runtimeInfo.buf);
	n_assert(err == VK_SUCCESS);

	// allocate a device memory backing for this
	uint32_t alignedSize;
	uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	flags |= info.sync == CoreGraphics::GpuBufferTypes::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
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

	if (0 != info.data)
	{
		n_assert((info.numVerts * vertexSize) == info.dataSize);
	}

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access, info.sync };
	runtimeInfo.layout = layout;
	loadInfo.vertexByteSize = vertexSize;
	loadInfo.vertexCount = info.numVerts;
	mapCount = 0;

	VertexBufferId ret;
	ret.id8 = VertexBufferIdType;
	ret.id24 = id;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.AsString());
#endif

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
const VertexBufferId
CreateVertexBuffer(const VertexBufferCreateDirectInfo& info)
{
	n_assert(CoreGraphics::GpuBufferTypes::UsageImmutable != info.usage)

	Ids::Id32 id = vboAllocator.Alloc();
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.Get<0>(id);
	VkVertexBufferRuntimeInfo& runtimeInfo = vboAllocator.Get<1>(id);
	uint32_t& mapCount = vboAllocator.Get<2>(id);

	loadInfo.dev = Vulkan::GetCurrentDevice();

	uint32_t qfamily = Vulkan::GetQueueFamily(GraphicsQueueType);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		info.size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,						// can only be accessed from the creator queue,
		0,												// number of queues in family
		nullptr											// array of queues belonging to family
	};

	VkResult err = vkCreateBuffer(loadInfo.dev, &bufinfo, NULL, &runtimeInfo.buf);
	n_assert(err == VK_SUCCESS);

	// allocate a device memory backing for this
	uint32_t alignedSize;
	uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	flags |= info.sync == CoreGraphics::GpuBufferTypes::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access, info.sync };
	runtimeInfo.layout = VertexLayoutId::Invalid();
	loadInfo.vertexByteSize = 1; // treat as a 1 byte vertex width * count bytes big buffer
	loadInfo.vertexCount = info.size;
	mapCount = 0;

	VertexBufferId ret;
	ret.id8 = VertexBufferIdType;
	ret.id24 = id;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.AsString());
#endif

	return ret;
}


//------------------------------------------------------------------------------
/**
*/
void
DestroyVertexBuffer(const VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.Get<0>(id.id24);
	VkVertexBufferRuntimeInfo& runtimeInfo = vboAllocator.Get<1>(id.id24);
	uint32_t& mapCount = vboAllocator.Get<2>(id.id24);

	n_assert(mapCount == 0);
	vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
	vkDestroyBuffer(loadInfo.dev, runtimeInfo.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void*
VertexBufferMap(const VertexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type)
{
	void* buf;
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.Get<0>(id.id24);
	uint32_t& mapCount = vboAllocator.Get<2>(id.id24);
	VkResult res = vkMapMemory(loadInfo.dev, loadInfo.mem, 0, VK_WHOLE_SIZE, 0, &buf);
	n_assert(res == VK_SUCCESS);
	mapCount++;
	return buf;
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUnmap(const VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.Get<0>(id.id24);
	uint32_t& mapCount = vboAllocator.Get<2>(id.id24);
	n_assert(mapCount > 0);
	vkUnmapMemory(loadInfo.dev, loadInfo.mem);
	mapCount--;
}


//------------------------------------------------------------------------------
/**
*/
const SizeT
VertexBufferGetNumVertices(const VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.Get<0>(id.id24);
	return loadInfo.vertexCount;
}

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
VertexBufferGetLayout(const VertexBufferId id)
{
	return vboAllocator.Get<1>(id.id24).layout;
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUpdate(const VertexBufferId id, void* data, PtrDiff size, PtrDiff offset)
{
	n_error("Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferLock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	n_error("Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUnlock(const VertexBufferId id, const PtrDiff offset, const PtrDiff range)
{
	n_error("Not implemented");
}

} // namespace CoreGraphics
