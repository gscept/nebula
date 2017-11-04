#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory vertex buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/memoryvertexbufferpoolbase.h"
#include "vkvertexbuffer.h"
namespace Vulkan
{
class VkMemoryVertexBufferPool : public Base::MemoryVertexBufferPoolBase
{
	__DeclareClass(VkMemoryVertexBufferPool);

public:

	/// bind vertex buffer
	void BindVertexBuffer(const Resources::ResourceId id, const IndexT slot, const IndexT offset);
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
		VkVertexBuffer::LoadInfo,					//0 loading stage info
		VkVertexBuffer::RuntimeInfo,				//1 runtime stage info
		Base::GpuResourceBase::GpuResourceMapInfo	//2 mapping stage info
	> allocator;
	__ImplementResourceAllocator(allocator);
};
} // namespace Vulkan