#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader variation (shader program within effect) in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "lowlevel/vk/vkprogram.h"
#include "lowlevel/vk/vkrenderstate.h"
#include "lowlevel/afxapi.h"
#include "resources/resourcepool.h"

namespace Vulkan
{
class VkShaderProgram
{
	struct VkShaderProgramRuntimeInfo;
	struct VkShaderProgramSetupInfo;
public:

	enum PipelineType
	{
		InvalidType,
		Compute,
		Graphics
	};

	/// applies program
	static void Apply(VkShaderProgramRuntimeInfo& info);

	/// discard variation
	static void Discard(VkShaderProgramSetupInfo& info, VkPipeline& computePipeline);

	/// get the number of vertex shader inputs
	static const uint32_t GetNumVertexInputs(AnyFX::VkProgram* program);
	/// get the number of pixel shader outputs
	static const uint32_t GetNumPixelOutputs(AnyFX::VkProgram* program);
private:

	friend class VkShader;
	friend class VkShaderPool;
	friend class VkPipelineDatabase;

	struct VkShaderProgramSetupInfo
	{
		VkDevice dev;
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

	struct VkShaderProgramRuntimeInfo
	{
		VkGraphicsPipelineCreateInfo info;
		VkPipeline pipeline;
		VkPipelineLayout layout;
		PipelineType type;
		uint32_t id;
	};

	static uint32_t uniqueIdCounter;
	typedef Ids::IdAllocator<
		VkShaderProgramSetupInfo,		//0 used for setup
		AnyFX::VkProgram*,				//1 program object
		VkShaderProgramRuntimeInfo		//2 used for runtime
	> ProgramAllocator;

	/// setup from AnyFX program
	static void Setup(const Ids::Id24 id, AnyFX::VkProgram* program, VkPipelineLayout& pipelineLayout, ProgramAllocator& allocator);

	/// create shader object
	static void CreateShader(const VkDevice dev, VkShaderModule* shader, unsigned binarySize, char* binary);
	/// create this program as a graphics program
	static void SetupAsGraphics(AnyFX::VkProgram* program, VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);
	/// create this program as a compute program (can be done immediately)
	static void SetupAsCompute(VkShaderProgramSetupInfo& setup, VkShaderProgramRuntimeInfo& runtime);

};


} // namespace Vulkan