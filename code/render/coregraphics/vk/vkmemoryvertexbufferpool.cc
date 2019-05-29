//------------------------------------------------------------------------------
// vkmemoryvertexbufferloader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkmemoryvertexbufferpool.h"
#include "coregraphics/vertexbuffer.h"
#include "vkgraphicsdevice.h"
#include "vkutilities.h"
#include "resources/resourcemanager.h"
#include "coregraphics/vertexsignaturepool.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryVertexBufferPool, 'VKVO', Resources::ResourceMemoryPool);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
VkMemoryVertexBufferPool::LoadFromMemory(const Resources::ResourceId id, const void* info)
{
	const VertexBufferCreateInfo* vboInfo = static_cast<const VertexBufferCreateInfo*>(info);
	n_assert(this->GetState(id) == Resource::Pending);
	n_assert(vboInfo->numVerts > 0);
	if (CoreGraphics::GpuBufferTypes::UsageImmutable == vboInfo->usage)
	{
		n_assert(0 != vboInfo->data);
		n_assert(0 < vboInfo->dataSize);
	}

	VkVertexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	VkVertexBufferRuntimeInfo& runtimeInfo = this->Get<1>(id.resourceId);
	uint32_t& mapCount = this->Get<2>(id.resourceId);

	loadInfo.dev = Vulkan::GetCurrentDevice();

	// create vertex layout
	VertexLayoutCreateInfo vertexLayoutCreateInfo =
	{
		vboInfo->comps,
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
		(uint32_t)(vertexSize * vboInfo->numVerts),
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
	flags |= vboInfo->sync == CoreGraphics::GpuBufferTypes::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(loadInfo.dev, runtimeInfo.buf, loadInfo.mem, 0);
	n_assert(err == VK_SUCCESS);

	if (vboInfo->data != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(loadInfo.dev, loadInfo.mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(vboInfo->dataSize <= (int32_t)alignedSize);
		memcpy(data, vboInfo->data, vboInfo->dataSize);
		vkUnmapMemory(loadInfo.dev, loadInfo.mem);
	}

	if (0 != vboInfo->data)
	{
		n_assert((vboInfo->numVerts * vertexSize) == vboInfo->dataSize);
	}

	// setup resource
	loadInfo.gpuResInfo = { vboInfo->usage, vboInfo->access, vboInfo->sync };
	runtimeInfo.layout = layout;
	loadInfo.vertexByteSize = vertexSize;
	loadInfo.vertexCount = vboInfo->numVerts;// = { vboInfo->numVerts, vertexSize };
	mapCount = 0;

	// set loaded flag
	this->states[id.poolId] = Resources::Resource::Loaded;

	return ResourcePool::Success;
}

//------------------------------------------------------------------------------
/**
*/
void
VkMemoryVertexBufferPool::Unload(const Resources::ResourceId id)
{
	VkVertexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	VkVertexBufferRuntimeInfo& runtimeInfo = this->Get<1>(id.resourceId);
	uint32_t& mapCount = this->Get<2>(id.resourceId);

	n_assert(mapCount == 0);
	vkFreeMemory(loadInfo.dev, loadInfo.mem, nullptr);
	vkDestroyBuffer(loadInfo.dev, runtimeInfo.buf, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
const SizeT
VkMemoryVertexBufferPool::GetNumVertices(const CoreGraphics::VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	return loadInfo.vertexCount;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::VertexLayoutId
VkMemoryVertexBufferPool::GetLayout(const CoreGraphics::VertexBufferId id)
{
	return this->Get<1>(id.resourceId).layout;
}

//------------------------------------------------------------------------------
/**
*/
void*
VkMemoryVertexBufferPool::Map(const CoreGraphics::VertexBufferId id, CoreGraphics::GpuBufferTypes::MapType mapType)
{
	void* buf;
	VkVertexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
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
VkMemoryVertexBufferPool::Unmap(const CoreGraphics::VertexBufferId id)
{
	VkVertexBufferLoadInfo& loadInfo = this->Get<0>(id.resourceId);
	uint32_t& mapCount = this->Get<2>(id.resourceId);
	n_assert(mapCount > 0);
	vkUnmapMemory(loadInfo.dev, loadInfo.mem);
	mapCount--;
}

} // namespace Vulkan