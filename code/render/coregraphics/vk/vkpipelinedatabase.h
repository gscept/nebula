#pragma once
//------------------------------------------------------------------------------
/**
	VkPipelineDatabase implements something akin to a VkPipelineCache.
	It works in a tree-like structure, where frequency of change dictates
	which level the state belongs to.

	It works by incrementally creating a DAG, which at the end points to 
	a path used to create a graphics pipeline. If it has already been created,
	then the pipeline is simply returned, however, if a node is set to initial
	(which happens on construction, meaning all children are also initial),
	then a new graphics pipeline is created and gets associated with the DAG path.
	
	It can be slightly more efficient, currently, whenever a state is changed high up in the DAG,
	child nodes are created for the rest of the currently used state. This is somewhat unnecessary,
	instead, the nodes should be created when and if we actually do create a pipeline. 

	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "coregraphics/shader.h"
#include "coregraphics/pass.h"

namespace Vulkan
{
class VkVertexLayout;
class VkShaderProgram;
class VkPass;
class VkPipelineDatabase
{
	__DeclareSingleton(VkPipelineDatabase);
public:

	enum StateLevel
	{
		PassLevel,
		SubpassLevel,
		ShaderLevel,
		VertexLayoutLevel,
		InputAssemblyLevel,
		PipelineLevel
	};

	/// constructor
	VkPipelineDatabase();
	/// destructor
	virtual ~VkPipelineDatabase();

	/// setup database
	void Setup(const VkDevice dev, const VkPipelineCache cache);

	/// set pass
	void SetPass(const CoreGraphics::PassId pass);
	/// set subpass
	void SetSubpass(uint32_t subpass);
	/// set shader
	void SetShader(const CoreGraphics::ShaderProgramId program);
	/// set vertex layout
	void SetVertexLayout(VkPipelineVertexInputStateCreateInfo* layout);
	/// set input layout
	void SetInputLayout(VkPipelineInputAssemblyStateCreateInfo* input);
	/// gets pipeline if it already exists, or creates if exists
	VkPipeline GetCompiledPipeline();
	/// resets all iterators
	void Reset();
private:
	
	VkDevice dev;
	VkPipelineCache cache;
	CoreGraphics::PassId currentPass;
	uint32_t currentSubpass;
	CoreGraphics::ShaderProgramId currentShaderProgram;
	VkPipelineVertexInputStateCreateInfo* currentVertexLayout;
	VkPipelineInputAssemblyStateCreateInfo* currentInputAssemblyInfo;
	StateLevel currentLevel;

	struct Tier1Node;
	struct Tier2Node;
	struct Tier3Node;
	struct Tier4Node;
	struct Tier5Node;

	struct BaseNode
	{
		bool initial;

		BaseNode() : initial(true) {}
	};
	struct Tier1Node : public BaseNode
	{
		Util::Dictionary<IndexT, Tier2Node*> children;
	};
	struct Tier2Node : public BaseNode
	{
		Util::Dictionary<uint32_t, Tier3Node*> children;
	};
	struct Tier3Node : public BaseNode
	{
		Util::Dictionary<VkPipelineVertexInputStateCreateInfo*, Tier4Node*> children;
	};
	struct Tier4Node : public BaseNode
	{
		Util::Dictionary<VkPipelineInputAssemblyStateCreateInfo*, Tier5Node*> children;
	};
	struct Tier5Node : public BaseNode
	{
		VkPipeline pipeline;
	};

	Util::Dictionary<CoreGraphics::PassId, Tier1Node*> tier1;
	IndexT tier1Iterator;

	Tier1Node* ct1;
	Tier2Node* ct2;
	Tier3Node* ct3;
	Tier4Node* ct4;
	Tier5Node* ct5;
	VkPipeline currentPipeline;
};
} // namespace Vulkan
