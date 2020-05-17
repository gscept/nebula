//------------------------------------------------------------------------------
//  vkvertexbuffer.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
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
	return vboAllocator.GetUnsafe<VertexBuffer_RuntimeInfo>(id.id24).buf;
}

//------------------------------------------------------------------------------
/**
*/
VkDeviceMemory 
VertexBufferGetVkMemory(const CoreGraphics::VertexBufferId id)
{
	return vboAllocator.GetUnsafe<VertexBuffer_LoadInfo>(id.id24).mem.mem;
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
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.GetUnsafe<VertexBuffer_LoadInfo>(id);
	VkVertexBufferRuntimeInfo& runtimeInfo = vboAllocator.GetUnsafe<VertexBuffer_RuntimeInfo>(id);
	VkVertexBufferMapInfo& mapInfo = vboAllocator.GetUnsafe<VertexBuffer_MapCount>(id);

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

	CoreGraphics::MemoryPoolType pool = CoreGraphics::BufferMemory_Local;
	if (info.mode == HostWriteable)
		pool = CoreGraphics::BufferMemory_Dynamic;
	else if (info.mode == HostMapped)
		pool = CoreGraphics::BufferMemory_Mapped;
	else if (info.mode == DeviceLocal)
		pool = CoreGraphics::BufferMemory_Local;

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
			if (pool == BufferMemory_Dynamic)
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

	if (0 != info.data)
	{
		n_assert((info.numVerts * vertexSize) == info.dataSize);
	}

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access };
	loadInfo.mode = info.mode;
	runtimeInfo.layout = layout;
	loadInfo.vertexByteSize = vertexSize;
	loadInfo.vertexCount = info.numVerts;
	mapInfo.mapCount = 0;

	VertexBufferId ret;
	ret.id8 = VertexBufferIdType;
	ret.id24 = id;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.Value());
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
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.GetUnsafe<VertexBuffer_LoadInfo>(id);
	VkVertexBufferRuntimeInfo& runtimeInfo = vboAllocator.GetUnsafe<VertexBuffer_RuntimeInfo>(id);
	VkVertexBufferMapInfo& mapInfo = vboAllocator.GetUnsafe<VertexBuffer_MapCount>(id);

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

	CoreGraphics::MemoryPoolType pool = CoreGraphics::BufferMemory_Local;
	if (info.mode == HostWriteable)
		pool = CoreGraphics::BufferMemory_Dynamic;
	else if (info.mode == HostMapped)
		pool = CoreGraphics::BufferMemory_Mapped;
	else if (info.mode == DeviceLocal)
		pool = CoreGraphics::BufferMemory_Local;

	// now bind memory to buffer
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
	loadInfo.mode = info.mode;
	runtimeInfo.layout = VertexLayoutId::Invalid();
	loadInfo.vertexByteSize = 1; // treat as a 1 byte vertex width * count bytes big buffer
	loadInfo.vertexCount = info.size;
	mapInfo.mapCount = 0;

	VertexBufferId ret;
	ret.id8 = VertexBufferIdType;
	ret.id24 = id;

#if NEBULA_GRAPHICS_DEBUG
	ObjectSetName(ret, info.name.Value());
#endif

	return ret;
}


//------------------------------------------------------------------------------
/**
*/
void
DestroyVertexBuffer(const VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.GetUnsafe<VertexBuffer_LoadInfo>(id.id24);
	VkVertexBufferRuntimeInfo& runtimeInfo = vboAllocator.GetUnsafe<VertexBuffer_RuntimeInfo>(id.id24);
	VkVertexBufferMapInfo& mapInfo = vboAllocator.GetUnsafe<2>(id.id24);

	n_assert(mapInfo.mapCount == 0);
	Vulkan::DelayedFreeMemory(loadInfo.mem);
	Vulkan::DelayedDeleteBuffer(runtimeInfo.buf);
	//vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
	//vkDestroyBuffer(loadInfo.dev, runtimeInfo.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void*
VertexBufferMap(const VertexBufferId id, const CoreGraphics::GpuBufferTypes::MapType type)
{
	VkVertexBufferMapInfo& mapInfo = vboAllocator.GetUnsafe<2>(id.id24);
	n_assert_fmt(mapInfo.mappedMemory != nullptr, "Index Buffer must be created as dynamic or mapped to support mapping");
	mapInfo.mapCount++;
	return mapInfo.mappedMemory;
}

//------------------------------------------------------------------------------
/**
*/
void
VertexBufferUnmap(const VertexBufferId id)
{
	VkVertexBufferMapInfo& mapInfo = vboAllocator.GetUnsafe<2>(id.id24);
	mapInfo.mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
void 
VertexBufferFlush(const VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.GetUnsafe<VertexBuffer_LoadInfo>(id.id24);
	Flush(loadInfo.dev, loadInfo.mem);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VertexBufferGetNumVertices(const VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = vboAllocator.GetUnsafe<VertexBuffer_LoadInfo>(id.id24);
	return loadInfo.vertexCount;
}

//------------------------------------------------------------------------------
/**
*/
const VertexLayoutId
VertexBufferGetLayout(const VertexBufferId id)
{
	return vboAllocator.GetUnsafe<VertexBuffer_RuntimeInfo>(id.id24).layout;
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
