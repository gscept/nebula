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
VkMemoryIndexBufferPool::UpdateResource(const Resources::ResourceId id, void* info)
{
	IndexBufferLoadInfo* iboInfo = static_cast<IndexBufferLoadInfo*>(info);
	const Ptr<CoreGraphics::IndexBuffer>& ibo = Resources::GetResource<CoreGraphics::IndexBuffer>(id);
	n_assert(ibo->GetState() == Resource::Pending);
	n_assert(iboInfo->indexType != IndexType::None);
	n_assert(iboInfo->numIndices > 0);
	if (IndexBuffer::UsageImmutable == iboInfo->usage)
	{
		n_assert(iboInfo->indexDataSize == (iboInfo->numIndices * IndexType::SizeOf(iboInfo->indexType)));
		n_assert(0 != iboInfo->indexDataPtr);
		n_assert(0 < iboInfo->indexDataSize);
	}

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

	VkBuffer buf;
	VkResult err = vkCreateBuffer(VkRenderDevice::dev, &bufinfo, NULL, &buf);
	n_assert(err == VK_SUCCESS);

	// allocate a device memory backing for this
	VkDeviceMemory mem;
	uint32_t alignedSize;
	uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	flags |= iboInfo->syncing == IndexBuffer::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(buf, mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(VkRenderDevice::dev, buf, mem, 0);
	n_assert(err == VK_SUCCESS);

	if (iboInfo->indexDataPtr != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(VkRenderDevice::dev, mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(iboInfo->indexDataSize <= (int32_t)alignedSize);
		memcpy(data, iboInfo->indexDataPtr, iboInfo->indexDataSize);
		vkUnmapMemory(VkRenderDevice::dev, mem);
	}

	// setup our IndexBuffer resource
	ibo->SetUsage(iboInfo->usage);
	ibo->SetAccess(iboInfo->access);
	ibo->SetSyncing(iboInfo->syncing);
	ibo->SetIndexType(iboInfo->indexType);
	ibo->SetNumIndices(iboInfo->numIndices);
	ibo->SetByteSize(iboInfo->numIndices * IndexType::SizeOf(iboInfo->indexType));
	ibo->SetVkBuffer(buf, mem);

	// if requested, create lock
	if (iboInfo->usage == IndexBuffer::UsageDynamic) ibo->CreateLock();

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryIndexBufferPool::Unload(const Ptr<Resources::Resource>& res)
{

}

} // namespace Vulkan