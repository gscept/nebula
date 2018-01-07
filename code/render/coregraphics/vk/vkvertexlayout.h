#pragma once
//------------------------------------------------------------------------------
/**
	Implements a vertex layout object used to construct a Vulkan pipeline.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/vertexlayoutbase.h"
#include "vkrenderdevice.h"
#include <array>

namespace Vulkan
{
class VkVertexLayout : public Base::VertexLayoutBase
{
public:

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

	/// setup the vertex layout
	static void Setup(BindInfo& info, VertexLayoutBaseInfo& baseInfo, VkPipelineVertexInputStateCreateInfo& vertexInfo, const Util::Array<CoreGraphics::VertexComponent>& c);
	/// discard the vertex layout object
	static void Discard();
	
	/// get derivative, and create if needed
	static VkPipelineVertexInputStateCreateInfo* GetDerivative(
		const AnyFX::VkProgram* program,
		const Ids::Id64 id,
		Util::HashTable<Ids::Id64, VkVertexLayout::DerivativeLayout*>& derivativeHashMap,
		Util::Array<VkVertexLayout::DerivativeLayout>& derivatives,
		VkVertexLayout::BindInfo& bindInfo,
		VkPipelineVertexInputStateCreateInfo& pipelineInfo
		);

	/// applies layout before rendering
	static void Apply();
private:
	friend class VkPipelineDatabase;
	friend class VkVertexSignaturePool;

	/// create derivative info from shader
	static VkPipelineVertexInputStateCreateInfo* CreateDerivative(
		const AnyFX::VkProgram* program, 
		const Ids::Id64 id,
		Util::HashTable<Ids::Id64, VkVertexLayout::DerivativeLayout*>& derivativeHashMap,
		Util::Array<VkVertexLayout::DerivativeLayout>& derivatives,
		VkVertexLayout::BindInfo& bindInfo,
		VkPipelineVertexInputStateCreateInfo& pipelineInfo
		);
};

} // namespace Vulkan