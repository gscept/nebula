#pragma once
//------------------------------------------------------------------------------
/**
	Implements an index buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/config.h"
#include "resources/resourcememorypool.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/indextype.h"
#include "coregraphics/indexbuffer.h"
#include "vulkan/vulkan.h"
namespace Vulkan
{
class VkMemoryIndexBufferPool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkMemoryIndexBufferPool);
public:

	/// bind index buffer
	void Bind(const CoreGraphics::IndexBufferId id, const IndexT offset);
	/// map the vertices for CPU access
	void* Map(const CoreGraphics::IndexBufferId id, CoreGraphics::GpuBufferTypes::MapType mapType);
	/// unmap the resource
	void Unmap(const CoreGraphics::IndexBufferId id);

	/// update resource
	LoadStatus LoadFromMemory(const Resources::ResourceId id, const void* info);
	/// unload resource
	void Unload(const Resources::ResourceId id);

	/// get index type for buffer
	CoreGraphics::IndexType::Code GetIndexType(const CoreGraphics::IndexBufferId id);
	/// get number of indices for buffer
	const SizeT GetNumIndices(const CoreGraphics::IndexBufferId id);
private:

	struct VkIndexBufferLoadInfo
	{
		VkDevice dev;
		VkDeviceMemory mem;
		CoreGraphics::GpuBufferTypes::SetupFlags gpuResInfo;
		uint32_t indexCount;
	};
	struct VkIndexBufferRuntimeInfo
	{
		VkBuffer buf;
		CoreGraphics::IndexType::Code type;
	};

	Ids::IdAllocator<
		VkIndexBufferLoadInfo,			//0 loading stage info
		VkIndexBufferRuntimeInfo,		//1 runtime stage info
		uint32_t						//2 mapping stage info
	> allocator;
	__ImplementResourceAllocatorTyped(allocator, IndexBufferIdType);
};
} // namespace Vulkan