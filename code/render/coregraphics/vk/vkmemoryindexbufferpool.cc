//------------------------------------------------------------------------------
// vkmemoryindexbufferloader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkmemoryindexbufferpool.h"
#include "coregraphics/indexbuffer.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
#include "resources/resourcemanager.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryIndexBufferPool, 'VKMI', Resources::ResourceMemoryPool);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkMemoryIndexBufferPool::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
	const IndexBufferCreateInfo* iboInfo = static_cast<const IndexBufferCreateInfo*>(info);
	n_assert(this->GetState(id) == Resource::Pending);
	n_assert(iboInfo->type != IndexType::None);
	n_assert(iboInfo->numIndices > 0);
	if (CoreGraphics::GpuBufferTypes::UsageImmutable == iboInfo->usage)
	{
		n_assert(iboInfo->dataSize == (iboInfo->numIndices * IndexType::SizeOf(iboInfo->type)));
		n_assert(0 != iboInfo->data);
		n_assert(0 < iboInfo->dataSize);
	}

	VkIndexBufferLoadInfo& loadInfo = this->Get<0>(id);
	VkIndexBufferRuntimeInfo& runtimeInfo = this->Get<1>(id);
	uint32_t& mapCount = this->Get<2>(id);

	loadInfo.dev = Vulkan::GetCurrentDevice();
	uint32_t qfamily = Vulkan::GetQueueFamily(GraphicsQueueType);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		(uint32_t)(iboInfo->numIndices * IndexType::SizeOf(iboInfo->type)),
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
	flags |= iboInfo->sync == CoreGraphics::GpuBufferTypes::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	if (iboInfo->data != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(loadInfo.dev, loadInfo.mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(iboInfo->dataSize <= (int32_t)alignedSize);
		memcpy(data, iboInfo->data, iboInfo->dataSize);
		vkUnmapMemory(loadInfo.dev, loadInfo.mem);
	}

	// setup resource
	loadInfo.gpuResInfo = { iboInfo->usage, iboInfo->access, iboInfo->sync };
	loadInfo.indexCount =  iboInfo->numIndices;
	runtimeInfo.type = iboInfo->type;
	mapCount = 0;

	// set loaded flag
	this->states[id.poolId] = Resources::Resource::Loaded;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryIndexBufferPool::Unload(const Resources::ResourceId id)
{
	VkIndexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	VkIndexBufferRuntimeInfo& runtimeInfo = this->Get<1>(id.resourceId);
	uint32_t& mapCount = this->Get<2>(id.resourceId);

	n_assert(mapCount == 0);
	vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
	vkDestroyBuffer(loadInfo.dev, runtimeInfo.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::IndexBufferId 
VkMemoryIndexBufferPool::CreateIndexBufferDirect(const Resources::ResourceId id, const CoreGraphics::IndexBufferCreateDirectInfo& info)
{
	n_assert(info.type != IndexType::None);
	n_assert(CoreGraphics::GpuBufferTypes::UsageImmutable != info.usage);

	VkIndexBufferLoadInfo& loadInfo = this->Get<0>(id);
	VkIndexBufferRuntimeInfo& runtimeInfo = this->Get<1>(id);
	uint32_t& mapCount = this->Get<2>(id);

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
	flags |= info.sync == CoreGraphics::GpuBufferTypes::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	// setup resource
	loadInfo.gpuResInfo = { info.usage, info.access, info.sync };
	loadInfo.indexCount = info.size; // fake size
	runtimeInfo.type = info.type;
	mapCount = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
VkMemoryIndexBufferPool::DestroyIndexBufferDirect(const Resources::ResourceId id)
{
	VkIndexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	VkIndexBufferRuntimeInfo& runtimeInfo = this->Get<1>(id.resourceId);
	uint32_t& mapCount = this->Get<2>(id.resourceId);

	n_assert(mapCount == 0);
	vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
	vkDestroyBuffer(loadInfo.dev, runtimeInfo.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::IndexType::Code
VkMemoryIndexBufferPool::GetIndexType(const CoreGraphics::IndexBufferId id)
{
	VkIndexBufferRuntimeInfo& runtimeInfo = this->Get<1>(id.resourceId);
	return runtimeInfo.type;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkMemoryIndexBufferPool::GetNumIndices(const CoreGraphics::IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	return loadInfo.indexCount;
}

//------------------------------------------------------------------------------
/**
*/
void*
VkMemoryIndexBufferPool::Map(const CoreGraphics::IndexBufferId id, CoreGraphics::GpuBufferTypes::MapType mapType)
{
	void* buf;
	VkIndexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	uint32_t& mapCount = this->Get<2>(id.resourceId);
	VkResult res = vkMapMemory(loadInfo.dev, loadInfo.mem, 0, VK_WHOLE_SIZE, 0, &buf);
	n_assert(res == VK_SUCCESS);
	mapCount++;
	return buf;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryIndexBufferPool::Unmap(const CoreGraphics::IndexBufferId id)
{
	VkIndexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	uint32_t& mapCount = this->Get<2>(id.resourceId);
	vkUnmapMemory(loadInfo.dev, loadInfo.mem);
	mapCount--;
}

} // namespace Vulkan