//------------------------------------------------------------------------------
// vkmemoryindexbufferloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkmemoryindexbufferloader.h"
#include "coregraphics/indexbuffer.h"
#include "vkrenderdevice.h"
#include "vkutilities.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryIndexBufferLoader, 'VKMI', Base::MemoryIndexBufferLoaderBase);

//------------------------------------------------------------------------------
/**
	FIXME: Go through the transfer queue if possible to update the index data
*/
bool
VkMemoryIndexBufferLoader::OnLoadRequested()
{
	n_assert(this->GetState() == Resource::Initial);
	n_assert(this->resource.isvalid());
	n_assert(!this->resource->IsAsyncEnabled());
	n_assert(this->indexType != IndexType::None);
	n_assert(this->numIndices > 0);
	if (IndexBuffer::UsageImmutable == this->usage)
	{
		n_assert(this->indexDataSize == (this->numIndices * IndexType::SizeOf(this->indexType)));
		n_assert(0 != this->indexDataPtr);
		n_assert(0 < this->indexDataSize);
	}

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		(uint32_t)(this->numIndices * IndexType::SizeOf(this->indexType)),
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
	flags |= this->syncing == IndexBuffer::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(buf, mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(VkRenderDevice::dev, buf, mem, 0);
	n_assert(err == VK_SUCCESS);

	if (this->indexDataPtr != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(VkRenderDevice::dev, mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(this->indexDataSize <= (int32_t)alignedSize);
		memcpy(data, this->indexDataPtr, this->indexDataSize);
		vkUnmapMemory(VkRenderDevice::dev, mem);
	}	

	// setup our IndexBuffer resource
	const Ptr<IndexBuffer>& res = this->resource.downcast<IndexBuffer>();
	n_assert(!res->IsLoaded());
	res->SetUsage(this->usage);
	res->SetAccess(this->access);
	res->SetSyncing(this->syncing);
	res->SetIndexType(this->indexType);
	res->SetNumIndices(this->numIndices);
	res->SetByteSize(this->numIndices * IndexType::SizeOf(this->indexType));
	res->SetVkBuffer(buf, mem);

	// if requested, create lock
	if (this->usage == IndexBuffer::UsageDynamic) res->CreateLock();

	// invalidate setup data (because we don't own our data)
	this->indexDataPtr = 0;
	this->indexDataSize = 0;

	this->SetState(Resource::Loaded);
	return true;
}

} // namespace Vulkan