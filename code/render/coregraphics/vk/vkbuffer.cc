//------------------------------------------------------------------------------
//  vkbuffer.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "vkgraphicsdevice.h"
#include "vkcommandbuffer.h"
#include "vkbuffer.h"
#include "vksubmissioncontext.h"
namespace Vulkan
{
VkBufferAllocator bufferAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
VkBuffer 
BufferGetVk(const CoreGraphics::BufferId id)
{
	return bufferAllocator.GetUnsafe<Buffer_RuntimeInfo>(id.id24).buf;
}

//------------------------------------------------------------------------------
/**
*/
VkDeviceMemory 
BufferGetVkMemory(const CoreGraphics::BufferId id)
{
	return bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24).mem.mem;
}

//------------------------------------------------------------------------------
/**
*/
VkDevice 
BufferGetVkDevice(const CoreGraphics::BufferId id)
{
	return bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24).dev;
}

} // namespace Vulkan
namespace CoreGraphics
{

using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
const BufferId 
CreateBuffer(const BufferCreateInfo& info)
{
	Ids::Id32 id = bufferAllocator.Alloc();
	VkBufferLoadInfo& loadInfo = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id);
	VkBufferRuntimeInfo& runtimeInfo = bufferAllocator.GetUnsafe<Buffer_RuntimeInfo>(id);
	VkBufferMapInfo& mapInfo = bufferAllocator.GetUnsafe<Buffer_MapInfo>(id);

	loadInfo.dev = Vulkan::GetCurrentDevice();
	runtimeInfo.usageFlags = info.usageFlags;

	VkBufferUsageFlags flags = 0;
	VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	Util::Set<uint32_t> queues;
	if (info.usageFlags & CoreGraphics::TransferBufferSource)
	{
		queues.Add(Vulkan::GetQueueFamily(GraphicsQueueType));
		queues.Add(Vulkan::GetQueueFamily(ComputeQueueType));
		queues.Add(Vulkan::GetQueueFamily(TransferQueueType));
		flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (info.usageFlags & CoreGraphics::TransferBufferDestination)
	{
		queues.Add(Vulkan::GetQueueFamily(GraphicsQueueType));
		queues.Add(Vulkan::GetQueueFamily(ComputeQueueType));
		queues.Add(Vulkan::GetQueueFamily(TransferQueueType));
		flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	if (info.usageFlags & CoreGraphics::ReadWriteBuffer)
	{
		queues.Add(Vulkan::GetQueueFamily(GraphicsQueueType));
		queues.Add(Vulkan::GetQueueFamily(ComputeQueueType));
		flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (info.usageFlags & CoreGraphics::ReadWriteTexelBuffer)
	{
		queues.Add(Vulkan::GetQueueFamily(GraphicsQueueType));
		queues.Add(Vulkan::GetQueueFamily(ComputeQueueType));
		flags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	}
	if (info.usageFlags & CoreGraphics::IndirectBuffer)
	{
		queues.Add(Vulkan::GetQueueFamily(GraphicsQueueType));
		queues.Add(Vulkan::GetQueueFamily(ComputeQueueType));
		flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	if (info.usageFlags & CoreGraphics::VertexBuffer)
		flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (info.usageFlags & CoreGraphics::IndexBuffer)
		flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (info.usageFlags & CoreGraphics::ConstantBuffer)
		flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (info.usageFlags & CoreGraphics::ConstantTexelBuffer)
		flags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

	if (info.mode == DeviceLocal && info.dataSize != 0)
		flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (queues.Size() > 1)
		sharingMode = VK_SHARING_MODE_CONCURRENT;

	// start by creating buffer
	uint size = info.byteSize == 0 ? info.size * info.elementSize : info.byteSize;
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,															// use for sparse buffers
		size,
		flags,
		sharingMode,												// can only be accessed from the creator queue,
		queues.Size(),												// number of queues in family
		queues.Size() > 0 ? queues.KeysAsArray().Begin() : nullptr	// array of queues belonging to family
	};

	VkResult err = vkCreateBuffer(loadInfo.dev, &bufinfo, NULL, &runtimeInfo.buf);
	n_assert(err == VK_SUCCESS);

	CoreGraphics::MemoryPoolType pool = CoreGraphics::MemoryPool_DeviceLocal;
	if (info.mode == DeviceLocal)
		pool = CoreGraphics::MemoryPool_DeviceLocal;
	else if (info.mode == HostLocal)
		pool = CoreGraphics::MemoryPool_HostLocal;
	else if (info.mode == DeviceToHost)
		pool = CoreGraphics::MemoryPool_DeviceToHost;
	else if (info.mode == HostToDevice)
		pool = CoreGraphics::MemoryPool_HostToDevice;

	// now bind memory to buffer
	CoreGraphics::Alloc alloc = AllocateMemory(loadInfo.dev, runtimeInfo.buf, pool);
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, alloc.mem, alloc.offset);
	n_assert(err == VK_SUCCESS);

	loadInfo.mem = alloc;

	if (info.mode == HostLocal || info.mode == HostToDevice || info.mode == DeviceToHost)
	{
		// copy contents and flush memory
		char* data = (char*)GetMappedMemory(alloc);
		mapInfo.mappedMemory = data;

		// if we have data, copy the memory to the region
		if (info.data)
		{
			n_assert(info.dataSize <= bufinfo.size);
			memcpy(data, info.data, info.dataSize);

			// if not host-local memory, we need to flush the initial update
			if (info.mode == HostToDevice || info.mode == DeviceToHost)
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
	else if (info.mode == DeviceLocal && info.data != nullptr)
	{
		// if device local and we provide a data pointer, create a temporary staging buffer and perform a copy
		VkBufferCreateInfo tempAllocInfo =
		{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			NULL,
			0,									// use for sparse buffers
			info.dataSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_SHARING_MODE_EXCLUSIVE,			// can only be accessed from the creator queue,
			0,									// number of queues in family
			nullptr								// array of queues belonging to family
		};

		// create temporary buffer
		VkBuffer tempBuffer;
		err = vkCreateBuffer(loadInfo.dev, &tempAllocInfo, nullptr, &tempBuffer);

		// allocate some host-local temporary memory for it
		CoreGraphics::Alloc tempAlloc = AllocateMemory(loadInfo.dev, tempBuffer, CoreGraphics::MemoryPool_HostLocal);
		err = vkBindBufferMemory(loadInfo.dev, tempBuffer, tempAlloc.mem, tempAlloc.offset);
		n_assert(err == VK_SUCCESS);

		// copy data to temporary buffer
		char* buf = (char*)GetMappedMemory(tempAlloc);
		memcpy(buf, info.data, info.dataSize);

		CoreGraphics::LockResourceSubmission();
		CoreGraphics::SubmissionContextId sub = CoreGraphics::GetResourceSubmissionContext();
		CoreGraphics::CommandBufferId cmd = SubmissionContextGetCmdBuffer(sub);
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = info.dataSize;

		// copy from temp buffer to source buffer in the resource submission context
		vkCmdCopyBuffer(Vulkan::CommandBufferGetVk(cmd), tempBuffer, runtimeInfo.buf, 1, &copy);

		// add delayed delete for this temporary buffer
		SubmissionContextFreeVkBuffer(sub, loadInfo.dev, tempBuffer);
		SubmissionContextFreeMemory(sub, tempAlloc);
		CoreGraphics::UnlockResourceSubmission();
	}

	// setup resource
	loadInfo.mode = info.mode;
	loadInfo.size = info.size;
	loadInfo.byteSize = alloc.size;
	loadInfo.elementSize = info.elementSize;
	mapInfo.mapCount = 0;

	BufferId ret;
	ret.id8 = BufferIdType;
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
DestroyBuffer(const BufferId id)
{
	VkBufferLoadInfo& loadInfo = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	VkBufferRuntimeInfo& runtimeInfo = bufferAllocator.GetUnsafe<Buffer_RuntimeInfo>(id.id24);
	VkBufferMapInfo& mapInfo = bufferAllocator.GetUnsafe<Buffer_MapInfo>(id.id24);

	n_assert(mapInfo.mapCount == 0);
	Vulkan::DelayedFreeMemory(loadInfo.mem);
	Vulkan::DelayedDeleteBuffer(runtimeInfo.buf);
	bufferAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const BufferUsageFlags 
BufferGetType(const BufferId id)
{
	return bufferAllocator.GetUnsafe<Buffer_RuntimeInfo>(id.id24).usageFlags;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
BufferGetSize(const BufferId id)
{
	VkBufferLoadInfo& loadInfo = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	return loadInfo.size;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
BufferGetElementSize(const BufferId id)
{
	VkBufferLoadInfo& loadInfo = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	return loadInfo.elementSize;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
BufferGetByteSize(const BufferId id)
{
	VkBufferLoadInfo& loadInfo = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	return loadInfo.byteSize;
}

//------------------------------------------------------------------------------
/**
*/
void* 
BufferMap(const BufferId id)
{
	VkBufferMapInfo& mapInfo = bufferAllocator.GetUnsafe<Buffer_MapInfo>(id.id24);
	n_assert_fmt(mapInfo.mappedMemory != nullptr, "Buffer must be created as dynamic or mapped to support mapping");
	mapInfo.mapCount++;
	return mapInfo.mappedMemory;
}

//------------------------------------------------------------------------------
/**
*/
void 
BufferUnmap(const BufferId id)
{
	VkBufferMapInfo& mapInfo = bufferAllocator.GetUnsafe<Buffer_MapInfo>(id.id24);
	mapInfo.mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
void
BufferUpdate(const BufferId id, const void* data, const uint size, const uint offset)
{
	VkBufferMapInfo& map = bufferAllocator.GetUnsafe<Buffer_MapInfo>(id.id24);

#if NEBULA_DEBUG
	VkBufferLoadInfo& setup = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	n_assert(size + offset <= (uint)setup.byteSize);
#endif
	byte* buf = (byte*)map.mappedMemory + offset;
	memcpy(buf, data, size);
}

//------------------------------------------------------------------------------
/**
*/
void
BufferUpdateArray(const BufferId id, const void* data, const uint size, const uint count, const uint offset)
{
	VkBufferMapInfo& map = bufferAllocator.GetUnsafe<Buffer_MapInfo>(id.id24);

#if NEBULA_DEBUG
	VkBufferLoadInfo& setup = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	n_assert(size + offset <= (uint)setup.byteSize);
#endif
	byte* buf = (byte*)map.mappedMemory + offset;
	memcpy(buf, data, size * count);
}

//------------------------------------------------------------------------------
/**
*/
void
BufferUpload(const BufferId id, const void* data, const uint size, const uint count, const uint offset, const CoreGraphics::SubmissionContextId sub)
{
	CoreGraphics::CommandBufferId cmd = SubmissionContextGetCmdBuffer(sub);

	uint numChunks = Math::n_divandroundup(size * count, 65536);
	int remainingBytes = size * count;
	int chunkOffset = offset;
	for (uint i = 0; i < numChunks; i++)
	{
		int chunkSize = Math::n_min(remainingBytes, 65536);
		vkCmdUpdateBuffer(Vulkan::CommandBufferGetVk(cmd), Vulkan::BufferGetVk(id), chunkOffset, chunkSize, data);
		chunkOffset += chunkSize;
		remainingBytes -= chunkSize;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
BufferUpdate(const BufferId id, const ConstantBufferAllocId alloc, const void* data, const uint size, const uint offset)
{
	VkBufferMapInfo& map = bufferAllocator.GetUnsafe<Buffer_MapInfo>(id.id24);

#if NEBULA_DEBUG
	VkBufferLoadInfo& setup = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	n_assert(size >= alloc.size);
	n_assert(size + offset + alloc.offset <= (uint)setup.byteSize);
#endif
	byte* buf = (byte*)map.mappedMemory + offset + alloc.offset;
	memcpy(buf, data, size);
}


//------------------------------------------------------------------------------
/**
*/
void
BufferFill(const BufferId id, char pattern, const CoreGraphics::SubmissionContextId sub)
{
	CoreGraphics::CommandBufferId cmd = SubmissionContextGetCmdBuffer(sub);
	VkBufferLoadInfo& setup = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	
	int remainingBytes = setup.byteSize;
	uint numChunks = Math::n_divandroundup(setup.byteSize, 65536);
	int chunkOffset = 0;
	for (uint i = 0; i < numChunks; i++)
	{
		int chunkSize = Math::n_min(remainingBytes, 65536);
		char* buf = n_new_array(char, chunkSize);
		memset(buf, pattern, chunkSize);
		vkCmdUpdateBuffer(Vulkan::CommandBufferGetVk(cmd), Vulkan::BufferGetVk(id), chunkOffset, chunkSize, buf);
		chunkOffset += chunkSize;
		remainingBytes -= chunkSize;
		n_delete_array(buf);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
BufferFlush(const BufferId id, IndexT offset, SizeT size)
{
	VkBufferLoadInfo& loadInfo = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	n_assert(size == NEBULA_WHOLE_BUFFER_SIZE ? true : (uint)offset + size <= loadInfo.byteSize);
	Flush(loadInfo.dev, loadInfo.mem, offset, size);
}

//------------------------------------------------------------------------------
/**
*/
void 
BufferInvalidate(const BufferId id, IndexT offset, SizeT size)
{
	VkBufferLoadInfo& loadInfo = bufferAllocator.GetUnsafe<Buffer_LoadInfo>(id.id24);
	n_assert(size == NEBULA_WHOLE_BUFFER_SIZE ? true : (uint)offset + size <= loadInfo.byteSize);
	Invalidate(loadInfo.dev, loadInfo.mem, offset, size);
}

} // namespace CoreGraphics
