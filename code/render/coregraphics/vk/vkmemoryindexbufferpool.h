#pragma once
//------------------------------------------------------------------------------
/**
	Implements an index buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/memoryindexbufferpoolbase.h"
#include "vkindexbuffer.h"
namespace Vulkan
{
class VkMemoryIndexBufferPool : public Base::MemoryIndexBufferPoolBase
{
	__DeclareClass(VkMemoryIndexBufferPool);
public:

	/// bind index buffer
	void BindIndexBuffer(const Resources::ResourceId id);
	/// map the vertices for CPU access
	void* Map(const Resources::ResourceId id, Base::GpuResourceBase::MapType mapType);
	/// unmap the resource
	void Unmap(const Resources::ResourceId id);

	/// update resource
	LoadStatus UpdateResource(const Ids::Id24 id, void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);
private:

	Ids::IdAllocator<
		VkIndexBuffer::LoadInfo,					//0 loading stage info
		VkIndexBuffer::RuntimeInfo,					//1 runtime stage info
		Base::GpuResourceBase::GpuResourceMapInfo	//2 mapping stage info
	> allocator;
	__ImplementResourceAllocator(allocator);
};
} // namespace Vulkan