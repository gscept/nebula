//------------------------------------------------------------------------------
// vkvertexbuffer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkvertexbuffer.h"
#include "vkrenderdevice.h"
#include "vkcmdbufferthread.h"
#include "vkutilities.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkVertexBuffer, 'VKVB', Base::VertexBufferBase);
//------------------------------------------------------------------------------
/**
*/
VkVertexBuffer::VkVertexBuffer() :
	mapcount(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkVertexBuffer::~VkVertexBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexBuffer::Unload()
{
	n_assert(this->mapcount == 0);
	vkFreeMemory(VkRenderDevice::dev, this->mem, NULL);
	vkDestroyBuffer(VkRenderDevice::dev, this->buf, NULL);
	this->vertexLayout = 0;
	VertexBufferBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void*
VkVertexBuffer::Map(MapType mapType)
{
	this->mapcount++;
	void* buf;
	VkResult res = vkMapMemory(VkRenderDevice::dev, this->mem, 0, this->vertexLayout->GetVertexByteSize() * this->numVertices, 0, &buf);
	n_assert(res == VK_SUCCESS);
	return buf;
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexBuffer::Unmap()
{
	vkUnmapMemory(VkRenderDevice::dev, this->mem);
	this->mapcount--;
}

//------------------------------------------------------------------------------
/**
*/
void
VkVertexBuffer::Update(const void* data, SizeT offset, SizeT length, void* mappedData)
{
	VkUtilities::BufferUpdate(VkRenderDevice::mainCmdDrawBuffer, this->buf, VkDeviceSize(offset), VkDeviceSize(length), data);
}

} // namespace Vulkan