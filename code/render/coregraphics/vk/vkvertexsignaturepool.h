#pragma once
//------------------------------------------------------------------------------
/**
	Implements a pool of vertex signatures (vertex shader input layouts) for Vulkan
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcememorypool.h"
#include "util/hashtable.h"
#include "resources/resourceid.h"
#include "coregraphics/base/vertexlayoutbase.h"
#include "vkvertexlayout.h"
namespace Vulkan
{
class VkVertexSignaturePool : public Resources::ResourceMemoryPool
{
	__DeclareClass(VkVertexSignaturePool);
public:
	/// constructor
	VkVertexSignaturePool();
	/// destructor
	virtual ~VkVertexSignaturePool();

	/// update resource
	LoadStatus LoadFromMemory(const Resources::ResourceId id, void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);

	/// bind layout
	void BindVertexLayout(const Resources::ResourceId id);
	/// get derivative
	VkPipelineVertexInputStateCreateInfo* GetDerivativeLayout(const Resources::ResourceId layout);
private:
	friend class VkMemoryVertexBufferPool;
	friend class VertexLayout;

	Ids::IdAllocator<
		Util::Array<VkPipelineVertexInputStateCreateInfo>,							//0 pipeline setup info
		Util::HashTable<Resources::ResourceId, VkVertexLayout::DerivativeLayout*>,	//1 program-to-derivative layout binding
		VkVertexLayout::BindInfo,													//2 setup info
		VertexLayoutInfo															//3 base info
	> allocator;
	__ImplementResourceAllocator(allocator);
};
} // namespace Vulkan