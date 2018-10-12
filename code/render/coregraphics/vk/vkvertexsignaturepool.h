#pragma once
//------------------------------------------------------------------------------
/**
	Implements a pool of vertex signatures (vertex shader input layouts) for Vulkan
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcememorypool.h"
#include "util/hashtable.h"
#include "resources/resourceid.h"
#include "coregraphics/base/vertexlayoutbase.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/config.h"

namespace CoreGraphics
{
	void SetVertexLayout(const CoreGraphics::VertexLayoutId& vl);
}

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
	LoadStatus LoadFromMemory(const Resources::ResourceId id, const void* info);
	/// unload resource
	void Unload(const Resources::ResourceId id);

	/// get byte size
	const SizeT GetVertexLayoutSize(const CoreGraphics::VertexLayoutId id);
	/// get components
	const Util::Array<CoreGraphics::VertexComponent>& GetVertexComponents(const CoreGraphics::VertexLayoutId id);
	/// get derivative
	VkPipelineVertexInputStateCreateInfo* GetDerivativeLayout(const CoreGraphics::VertexLayoutId layout, const CoreGraphics::ShaderProgramId shader);
private:
	friend class VkMemoryVertexBufferPool;
	friend class VertexLayout;

	friend void	CoreGraphics::SetVertexLayout(const CoreGraphics::VertexLayoutId& vl);

	ID_64_TYPE(DerivativeId);

	Ids::IdAllocator<
		Util::HashTable<uint64_t, DerivativeLayout>,		//0 program-to-derivative layout binding
		VkPipelineVertexInputStateCreateInfo,				//1 base vertex input state
		BindInfo,											//2 setup info
		CoreGraphics::VertexLayoutInfo						//3 base info
	> allocator;
	__ImplementResourceAllocatorTyped(allocator, VertexLayoutIdType);
};
} // namespace Vulkan