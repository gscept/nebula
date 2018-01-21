#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory vertex buffer loader for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/config.h"
#include "resources/resourcememorypool.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/vertexbuffer.h"
namespace Vulkan
{
class VkMemoryVertexBufferPool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkMemoryVertexBufferPool);

public:

	/// bind vertex buffer
	void Bind(const CoreGraphics::VertexBufferId id, const IndexT slot, const IndexT offset);
	/// map the vertices for CPU access
	void* Map(const CoreGraphics::VertexBufferId id, CoreGraphics::GpuBufferTypes::MapType mapType);
	/// unmap the resource
	void Unmap(const CoreGraphics::VertexBufferId id);

	/// update resource
	LoadStatus LoadFromMemory(const Ids::Id24 id, const void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);

	/// get number of vertices
	const SizeT GetNumVertices(const CoreGraphics::VertexBufferId id);
private:

	struct VkVertexBufferLoadInfo
	{
		VkDevice dev;
		VkDeviceMemory mem;
		CoreGraphics::GpuBufferTypes::SetupFlags gpuResInfo;
		uint32_t vertexCount;
		uint32_t vertexByteSize;
	};

	struct VkVertexBufferRuntimeInfo
	{
		VkBuffer buf;
		CoreGraphics::VertexLayoutId layout;
	};

	Ids::IdAllocator<
		VkVertexBufferLoadInfo,			//0 loading stage info
		VkVertexBufferRuntimeInfo,		//1 runtime stage info
		uint32_t						//2 mapping stage info
	> allocator;
	__ImplementResourceAllocatorTyped(allocator, VertexBufferIdType);
};
} // namespace Vulkan