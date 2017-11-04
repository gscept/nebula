#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader variation (shader program within effect) in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/shadervariationbase.h"
#include "lowlevel/vk/vkprogram.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "lowlevel/afxapi.h"
#include "resources/resourcepool.h"

namespace Vulkan
{
class VkShaderProgram : public Base::ShaderVariationBase
{
	__DeclareClass(VkShaderProgram);
	struct SetupInfo;
	struct RuntimeInfo;
public:

	enum PipelineType
	{
		InvalidType,
		Compute,
		Graphics
	};

	/// constructor
	VkShaderProgram();
	/// destructor
	virtual ~VkShaderProgram();

	/// applies program
	static void Apply(RuntimeInfo& info);

	/// discard variation
	static void Discard(SetupInfo& info, VkPipeline& computePipeline);

	/// get the number of vertex shader inputs
	static const uint32_t GetNumVertexInputs(AnyFX::VkProgram* program);
	/// get the number of pixel shader outputs
	static const uint32_t GetNumPixelOutputs(AnyFX::VkProgram* program);
private:

	friend class VkShader;
	friend class VkShaderPool;
	friend class VkPipelineDatabase;

	struct SetupInfo
	{
		VkPipelineVertexInputStateCreateInfo vertexInfo;
		VkPipelineRasterizationStateCreateInfo rasterizerInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDynamicStateCreateInfo dynamicInfo;
		VkPipelineTessellationStateCreateInfo tessInfo;
		VkPipelineShaderStageCreateInfo shaderInfos[5];
		VkShaderModule vs, hs, ds, gs, ps, cs;
		Util::String name;
		CoreGraphics::ShaderFeature::Mask mask;
	};

	struct RuntimeInfo
	{
		VkGraphicsPipelineCreateInfo info;
		VkPipeline pipeline;
		VkPipelineLayout layout;
		PipelineType type;
		uint32_t id;
	};

	static uint32_t uniqueIdCounter;
	typedef Ids::IdAllocator<
		SetupInfo,						//0 used for setup
		AnyFX::VkProgram*,				//1 program object
		RuntimeInfo						//2 used for runtime
	> ProgramAllocator;

	/// setup from AnyFX program
	static void Setup(const Ids::Id24 id, AnyFX::VkProgram* program, VkPipelineLayout& pipelineLayout, ProgramAllocator& allocator);

	/// create shader object
	static void CreateShader(VkShaderModule* shader, unsigned binarySize, char* binary);
	/// create this program as a graphics program
	static void SetupAsGraphics(AnyFX::VkProgram* program, SetupInfo& setup, RuntimeInfo& runtime);
	/// create this program as a compute program (can be done immediately)
	static void SetupAsCompute(SetupInfo& setup, RuntimeInfo& runtime);
};


} // namespace Vulkan