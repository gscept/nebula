#pragma once
//------------------------------------------------------------------------------
/**
	Implements an index buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcememorypool.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/indextype.h"
#include "vulkan/vulkan.h"
namespace Vulkan
{
class VkMemoryIndexBufferPool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkMemoryIndexBufferPool);
public:

	/// bind index buffer
	void IndexBufferBind(const Resources::ResourceId id, const IndexT offset);
	/// map the vertices for CPU access
	void* Map(const Resources::ResourceId id, CoreGraphics::GpuBufferTypes::MapType mapType);
	/// unmap the resource
	void Unmap(const Resources::ResourceId id);

	/// update resource
	LoadStatus LoadFromMemory(const Ids::Id24 id, const void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);
private:

	struct LoadInfo
	{
		VkDevice dev;
		VkDeviceMemory mem;
		CoreGraphics::GpuBufferTypes::SetupFlags gpuResInfo;
		uint32_t indexCount;
	};
	struct RuntimeInfo
	{
		VkBuffer buf;
		CoreGraphics::IndexType::Code type;
	};

	Ids::IdAllocator<
		LoadInfo,			//0 loading stage info
		RuntimeInfo,		//1 runtime stage info
		uint32_t			//2 mapping stage info
	> allocator;
	__ImplementResourceAllocator(allocator);
};
} // namespace Vulkan