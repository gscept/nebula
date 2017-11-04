//------------------------------------------------------------------------------
// vkmemoryvertexbufferloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkmemoryvertexbufferpool.h"
#include "coregraphics/vertexlayoutserver.h"
#include "coregraphics/vertexbuffer.h"
#include "vkrenderdevice.h"
#include "vkutilities.h"
#include "resources/resourcemanager.h"
#include "coregraphics/vertexsignaturepool.h"
#include "coregraphics/coregraphics.h"
#include "../base/gpuresourcebase.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryVertexBufferPool, 'VKVO', Base::MemoryVertexBufferPoolBase);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkMemoryVertexBufferPool::UpdateResource(const Ids::Id24 id, void* info)
{
	VertexBufferLoadInfo* vboInfo = static_cast<VertexBufferLoadInfo*>(info);
	n_assert(this->GetState(id) == Resource::Pending);
	n_assert(vboInfo->numVertices > 0);
	if (Base::GpuResourceBase::UsageImmutable == vboInfo->usage)
	{
		n_assert(0 != vboInfo->vertexDataPtr);
		n_assert(0 < vboInfo->vertexDataSize);
	}
	SizeT vertexSize = VertexLayoutServer::Instance()->CalculateVertexSize(vboInfo->vertexComponents);

	VkVertexBuffer::LoadInfo& loadInfo = this->Get<0>(id);
	VkVertexBuffer::RuntimeInfo& runtimeInfo = this->Get<1>(id);
	Base::GpuResourceBase::GpuResourceMapInfo& mapInfo = this->Get<2>(id);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		(uint32_t)(vertexSize * vboInfo->numVertices),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE,						// can only be accessed from the creator queue,
		1,												// number of queues in family
		&VkRenderDevice::Instance()->drawQueueFamily	// array of queues belonging to family
	};

	VkResult err = vkCreateBuffer(VkRenderDevice::dev, &bufinfo, NULL, &runtimeInfo.buf);
	n_assert(err == VK_SUCCESS);

	// allocate a device memory backing for this
	VkDeviceMemory mem;
	uint32_t alignedSize;
	uint32_t flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	flags |= vboInfo->syncing == Base::GpuResourceBase::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(VkRenderDevice::dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	if (vboInfo->vertexDataPtr != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(VkRenderDevice::dev, loadInfo.mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(vboInfo->vertexDataSize <= (int32_t)alignedSize);
		memcpy(data, vboInfo->vertexDataPtr, vboInfo->vertexDataSize);
		vkUnmapMemory(VkRenderDevice::dev, loadInfo.mem);
	}

	// create signature and resource
	Util::StringAtom vertexSignature = VertexLayout::BuildSignature(vboInfo->vertexComponents);
	Resources::ResourceId layout = CoreGraphics::layoutPool->ReserveResource(vertexSignature, "system");
	CoreGraphics::layoutPool->UpdateResource(layout, &vboInfo->vertexComponents);
	if (0 != vboInfo->vertexDataPtr)
	{
		n_assert((vboInfo->numVertices * vertexSize) == vboInfo->vertexDataSize);
	}

	// setup resource
	loadInfo.gpuResInfo = { vboInfo->usage, vboInfo->access, vboInfo->syncing };
	runtimeInfo.layout = layout;
	loadInfo.vboInfo = { vboInfo->numVertices, vertexSize };
	mapInfo.mapCount = 0;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryVertexBufferPool::Unload(const Ids::Id24 id)
{
	VkVertexBuffer::LoadInfo& loadInfo = this->Get<0>(id);
	VkVertexBuffer::RuntimeInfo& runtimeInfo = this->Get<1>(id);
	Base::GpuResourceBase::GpuResourceMapInfo& mapInfo = this->Get<2>(id);

	n_assert(mapInfo.mapCount == 0);
	vkFreeMemory(VkRenderDevice::dev, loadInfo.mem, nullptr);
	vkDestroyBuffer(VkRenderDevice::dev, runtimeInfo.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryVertexBufferPool::BindVertexBuffer(const Resources::ResourceId id, const IndexT slot, const IndexT offset)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkVertexBuffer::RuntimeInfo& runtimeInfo = this->Get<1>(resId);
	VkRenderDevice::Instance()->SetStreamVertexBuffer(slot, runtimeInfo.buf, offset);
}

//------------------------------------------------------------------------------
/**
*/
void*
VkMemoryVertexBufferPool::Map(const Resources::ResourceId id, Base::GpuResourceBase::MapType mapType)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	void* buf;
	VkVertexBuffer::LoadInfo& loadInfo = this->Get<0>(resId);
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
VkMemoryVertexBufferPool::Unmap(const Resources::ResourceId id)
{
	const Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(id));
	VkVertexBuffer::LoadInfo& loadInfo = this->Get<0>(resId);
	Base::GpuResourceBase::GpuResourceMapInfo& mapInfo = this->Get<2>(resId);
	vkUnmapMemory(VkRenderDevice::dev, loadInfo.mem);
	mapInfo.mapCount--;
}

} // namespace Vulkan