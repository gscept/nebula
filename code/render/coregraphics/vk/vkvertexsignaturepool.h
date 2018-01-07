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
#include "coregraphics/vertexlayout.h"
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

	struct DerivativeLayout
	{
		VkPipelineVertexInputStateCreateInfo info;
		Util::Array<VkVertexInputAttributeDescription> attrs;
	};

	struct BindInfo
	{
		Util::FixedArray<VkVertexInputBindingDescription> binds;
		Util::FixedArray<VkVertexInputAttributeDescription> attrs;
	};


	/// update resource
	LoadStatus LoadFromMemory(const Ids::Id24 id, void* info);
	/// unload resource
	void Unload(const Ids::Id24 id);

	/// bind layout
	void VertexLayoutBind(const CoreGraphics::VertexLayoutId id);
	/// get byte size
	const SizeT VertexLayoutGetSize(const CoreGraphics::VertexLayoutId id);
	/// get derivative
	VkPipelineVertexInputStateCreateInfo* GetDerivativeLayout(const CoreGraphics::VertexLayoutId layout, const CoreGraphics::ShaderProgramId shader);
private:
	friend class VkMemoryVertexBufferPool;
	friend class VertexLayout;

	ID_64_TYPE(DerivativeId);

	Ids::IdAllocator<
		Util::HashTable<DerivativeId, DerivativeLayout>,		//0 program-to-derivative layout binding
		VkPipelineVertexInputStateCreateInfo,				//1 base vertex input state
		BindInfo,											//2 setup info
		CoreGraphics::VertexLayoutInfo						//3 base info
	> allocator;
	__ImplementResourceAllocator(allocator);
};
} // namespace Vulkan