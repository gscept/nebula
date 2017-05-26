//------------------------------------------------------------------------------
// vkindexbuffer.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkindexbuffer.h"
#include "vkrenderdevice.h"
#include "coregraphics/indextype.h"
#include "vkutilities.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkIndexBuffer, 'VKIB', Base::IndexBufferBase);
//------------------------------------------------------------------------------
/**
*/
VkIndexBuffer::VkIndexBuffer() :
	mapcount(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkIndexBuffer::~VkIndexBuffer()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkIndexBuffer::Unload()
{
	n_assert(this->mapcount == 0);
	vkFreeMemory(VkRenderDevice::dev, this->mem, NULL);
	vkDestroyBuffer(VkRenderDevice::dev, this->buf, NULL);
	IndexBufferBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void*
VkIndexBuffer::Map(MapType mapType)
{
	this->mapcount++;
	void* buf;
	VkResult res = vkMapMemory(VkRenderDevice::dev, this->mem, 0, this->numIndices * CoreGraphics::IndexType::SizeOf(this->indexType), 0, &buf);
	n_assert(res == VK_SUCCESS);
	return buf;
}

//------------------------------------------------------------------------------
/**
*/
void
VkIndexBuffer::Unmap()
{
	vkUnmapMemory(VkRenderDevice::dev, this->mem);
	this->mapcount--;
}

//------------------------------------------------------------------------------
/**
*/
void
VkIndexBuffer::Update(const void* data, SizeT offset, SizeT length, void* mappedData)
{
	// send update 
	VkUtilities::BufferUpdate(VkRenderDevice::mainCmdDrawBuffer, this->buf, VkDeviceSize(offset), VkDeviceSize(length), data);
}

} // namespace Vulkan