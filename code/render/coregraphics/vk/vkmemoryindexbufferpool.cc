//------------------------------------------------------------------------------
// vkmemoryindexbufferloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkmemoryindexbufferpool.h"
#include "coregraphics/indexbuffer.h"
#include "vkrenderdevice.h"
#include "vkutilities.h"
#include "resources/resourcemanager.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryIndexBufferPool, 'VKMI', Base::MemoryIndexBufferPoolBase);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkMemoryIndexBufferPool::UpdateResource(const Ids::Id24 id, void* info)
{
	IndexBufferLoadInfo* iboInfo = static_cast<IndexBufferLoadInfo*>(info);
	n_assert(this->GetState(id) == Resource::Pending);
	n_assert(iboInfo->indexType != IndexType::None);
	n_assert(iboInfo->numIndices > 0);
	if (Base::GpuResourceBase::UsageImmutable == iboInfo->usage)
	{
		n_assert(iboInfo->indexDataSize == (iboInfo->numIndices * IndexType::SizeOf(iboInfo->indexType)));
		n_assert(0 != iboInfo->indexDataPtr);
		n_assert(0 < iboInfo->indexDataSize);
	}

	VkIndexBuffer::LoadInfo& loadInfo = this->Get<0>(id);
	VkIndexBuffer::RuntimeInfo& runtimeInfo = this->Get<1>(id);
	Base::GpuResourceBase::GpuResourceMapInfo& mapInfo = this->Get<2>(id);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		(uint32_t)(iboInfo->numIndices * IndexType::SizeOf(iboInfo->indexType)),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,						// can only be accessed from the creator queue,
		1,												// number of queues in family
		&VkRenderDevice::Instance()->drawQueueFamily	// array of queues belonging to family
	};

	VkResult err = vkCreateBuffer(VkRenderDevice::dev, &bufinfo, NULL, &runtimeInfo.buf);
	n_assert(err == VK_SUCCESS);

	// allocate a device memory backing for this
	uint32_t alignedSize;
	uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	flags |= iboInfo->syncing == Base::GpuResourceBase::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(VkRenderDevice::dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	if (iboInfo->indexDataPtr != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(VkRenderDevice::dev, loadInfo.mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(iboInfo->indexDataSize <= (int32_t)alignedSize);
		memcpy(data, iboInfo->indexDataPtr, iboInfo->indexDataSize);
		vkUnmapMemory(VkRenderDevice::dev, loadInfo.mem);
	}

	// setup resource
	loadInfo.gpuResInfo = { iboInfo->usage, iboInfo->access, iboInfo->syncing };
	loadInfo.iboInfo = { iboInfo->numIndices };
	runtimeInfo.type = iboInfo->indexType;
	mapInfo.mapCount = 0;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryIndexBufferPool::Unload(const Ids::Id24 id)
{
	VkIndexBuffer::LoadInfo& loadInfo = this->Get<0>(id);
	VkIndexBuffer::RuntimeInfo& runtimeInfo = this->Get<1>(id);
	Base::GpuResourceBase::GpuResourceMapInfo& mapInfo = this->Get<2>(id);

	n_assert(mapInfo.mapCount == 0);
	vkFreeMemory(VkRenderDevice::dev, loadInfo.mem, nullptr);
	vkDestroyBuffer(VkRenderDevice::dev, runtimeInfo.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryIndexBufferPool::BindIndexBuffer(const Resources::ResourceId id)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkIndexBuffer::RuntimeInfo& runtimeInfo = this->Get<1>(resId);
	VkRenderDevice::Instance()->SetIndexBuffer(runtimeInfo.buf, runtimeInfo.type);
}

//------------------------------------------------------------------------------
/**
*/
void*
VkMemoryIndexBufferPool::Map(const Resources::ResourceId id, Base::GpuResourceBase::MapType mapType)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	void* buf;
	VkIndexBuffer::LoadInfo& loadInfo = this->Get<0>(resId);
	Base::GpuResourceBase::GpuResourceMapInfo& mapInfo = this->Get<2>(resId);
	VkResult res = vkMapMemory(VkRenderDevice::dev, loadInfo.mem, 0, VK_WHOLE_SIZE, 0, &buf);
	n_assert(res == VK_SUCCESS);
	mapInfo.mapCount++;
	return buf;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryIndexBufferPool::Unmap(const Resources::ResourceId id)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkIndexBuffer::LoadInfo& loadInfo = this->Get<0>(resId);
	Base::GpuResourceBase::GpuResourceMapInfo& mapInfo = this->Get<2>(resId);
	vkUnmapMemory(VkRenderDevice::dev, loadInfo.mem);
	mapInfo.mapCount--;
}

} // namespace Vulkan