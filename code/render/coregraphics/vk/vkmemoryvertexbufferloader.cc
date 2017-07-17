//------------------------------------------------------------------------------
// vkmemoryvertexbufferloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkmemoryvertexbufferloader.h"
#include "coregraphics/vertexlayoutserver.h"
#include "coregraphics/vertexbuffer.h"
#include "vkrenderdevice.h"
#include "vkutilities.h"
#include "resources/resourcemanager.h"

using namespace CoreGraphics;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkMemoryVertexBufferLoader, 'VKVO', Base::MemoryVertexBufferLoaderBase);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceMemoryLoader::LoadStatus
VkMemoryVertexBufferLoader::Load(const Resources::ResourceId id)
{
	const Ptr<CoreGraphics::VertexBuffer>& vbo = Resources::GetResource<CoreGraphics::VertexBuffer>(id);
	n_assert(vbo->GetState() == Resource::Pending);
	n_assert(this->numVertices > 0);
	if (VertexBuffer::UsageImmutable == this->usage)
	{
		n_assert(0 != this->vertexDataPtr);
		n_assert(0 < this->vertexDataSize);
	}
	SizeT vertexSize = VertexLayoutServer::Instance()->CalculateVertexSize(this->vertexComponents);

	// start by creating buffer
	VkBufferCreateInfo bufinfo =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		NULL,
		0,					// use for sparse buffers
		(uint32_t)(vertexSize * this->numVertices),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
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
	flags |= this->syncing == VertexBuffer::SyncingCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0;
	VkUtilities::AllocateBufferMemory(buf, mem, VkMemoryPropertyFlagBits(flags), alignedSize);

	// now bind memory to buffer
	err = vkBindBufferMemory(VkRenderDevice::dev, buf, mem, 0);
	n_assert(err == VK_SUCCESS);

	if (this->vertexDataPtr != 0)
	{
		// map memory so we can initialize it
		void* data;
		err = vkMapMemory(VkRenderDevice::dev, mem, 0, alignedSize, 0, &data);
		n_assert(err == VK_SUCCESS);
		n_assert(this->vertexDataSize <= (int32_t)alignedSize);
		memcpy(data, this->vertexDataPtr, this->vertexDataSize);
		vkUnmapMemory(VkRenderDevice::dev, mem);
	}

	Ptr<VertexLayout> vertexLayout = VkVertexLayoutServer::Instance()->CreateSharedVertexLayout(this->vertexComponents);
	//Ptr<VertexLayout> vertexLayout = VertexLayout::Create();
	//vertexLayout->Setup(this->vertexComponents);
	if (0 != this->vertexDataPtr)
	{
		n_assert((this->numVertices * vertexLayout->GetVertexByteSize()) == this->vertexDataSize);
	}

	// setup our resource object
	vbo->SetUsage(this->usage);
	vbo->SetAccess(this->access);
	vbo->SetSyncing(this->syncing);
	vbo->SetVertexLayout(vertexLayout);
	vbo->SetNumVertices(this->numVertices);
	vbo->SetByteSize(vertexSize * this->numVertices);
	vbo->SetVkBuffer(buf, mem);

	// if requested, create lock
	//if (this->usage == VertexBuffer::UsageDynamic) res->CreateLock();

	// invalidate setup data (because we don't own our data)
	this->vertexDataPtr = 0;
	this->vertexDataSize = 0;

	return ResourceMemoryLoader::Success;
}

} // namespace Vulkan