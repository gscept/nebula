#pragma once
//------------------------------------------------------------------------------
/**
	Implements a memory vertex buffer loader for Vulkan.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/config.h"
#include "resources/resourcememorypool.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/vertexbuffer.h"

namespace CoreGraphics
{
	void SetStreamVertexBuffer(IndexT streamIndex, const CoreGraphics::VertexBufferId& vb, IndexT offsetVertexIndex);
}
namespace Vulkan
{
class VkMemoryVertexBufferPool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkMemoryVertexBufferPool);

public:

	/// map the vertices for CPU access
	void* Map(const CoreGraphics::VertexBufferId id, CoreGraphics::GpuBufferTypes::MapType mapType);
	/// unmap the resource
	void Unmap(const CoreGraphics::VertexBufferId id);

	/// update resource
	LoadStatus LoadFromMemory(const Resources::ResourceId id, const void* info) override;
	/// unload resource
	void Unload(const Resources::ResourceId id) override;

	/// get number of vertices
	const SizeT GetNumVertices(const CoreGraphics::VertexBufferId id);
	/// get layout
	const CoreGraphics::VertexLayoutId GetLayout(const CoreGraphics::VertexBufferId id);
private:
	friend void	CoreGraphics::SetStreamVertexBuffer(IndexT streamIndex, const CoreGraphics::VertexBufferId& vb, IndexT offsetVertexIndex);

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