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

namespace Vulkan
{
class VkShaderProgram : public Base::ShaderVariationBase
{
	__DeclareClass(VkShaderProgram);
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
	void Apply();
	/// performs a variable commit to the current program
	void Commit();

	/// discard variation
	void Discard();

	/// get the number of vertex shader inputs
	const uint32_t GetNumVertexInputs() const;
	/// get the number of pixel shader outputs
	const uint32_t GetNumPixelOutputs() const;
	/// get AnyFX program backend
	const AnyFX::VkProgram* GetVkProgram() const;
	/// get unique id
	const uint32_t GetUniqueId() const;
	/// get type of shader program
	const PipelineType& GetPipelineType() const;
private:

	friend class VkShader;
	friend class VkShaderLoader;
	friend class VkPipelineDatabase;

	/// setup from AnyFX program
	void Setup(AnyFX::VkProgram* program, const VkPipelineLayout& layout);

	/// create shader object
	void CreateShader(VkShaderModule* shader, unsigned binarySize, char* binary);
	/// create this program as a graphics program
	void SetupAsGraphics();
	/// create this program as a compute program (can be done immediately)
	void SetupAsCompute();
	/// create this program as empty
	void SetupAsEmpty();

	AnyFX::VkProgram* program;
	uint32_t uniqueId;

	static uint32_t uniqueIdCounter;

	Util::Array<VkSampler> immutableSamplers;
	VkPushConstantRange constantRange;

	VkShaderModule vs, hs, ds, gs, ps, cs;

	VkPipeline computePipeline;
	VkPipelineLayout pipelineLayout;

	VkPipelineVertexInputStateCreateInfo vertexInfo;
	VkPipelineRasterizationStateCreateInfo rasterizerInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDynamicStateCreateInfo dynamicInfo;
	VkPipelineTessellationStateCreateInfo tessInfo;
	VkPipelineShaderStageCreateInfo shaderInfos[5];
	VkGraphicsPipelineCreateInfo shaderPipelineInfo;
	PipelineType pipelineType;
};

//------------------------------------------------------------------------------
/**
*/
inline const AnyFX::VkProgram*
VkShaderProgram::GetVkProgram() const
{
	return this->program;
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
VkShaderProgram::GetUniqueId() const
{
	return this->uniqueId;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkShaderProgram::PipelineType&
VkShaderProgram::GetPipelineType() const
{
	return this->pipelineType;
}

} // namespace Vulkan