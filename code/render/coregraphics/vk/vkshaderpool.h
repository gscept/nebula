#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader loader from stream into a Vulkan shader.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/config.h"
#include "resources/resourcestreampool.h"
#include "util/dictionary.h"
#include "coregraphics/shaderfeature.h"
#include "lowlevel/vk/vkprogram.h"
#include "coregraphics/shader.h"
#include "vulkan/vulkan.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/shaderidentifier.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/resourcetable.h"
#include "vkshaderprogram.h"
#include "vktexture.h"
#include "vkshader.h"

namespace CoreGraphics
{
	void SetShaderProgram(const CoreGraphics::ShaderProgramId& pro);
	void SetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
}

namespace Vulkan
{

class VkShaderPool : public Resources::ResourceStreamPool
{
	__DeclareClass(VkShaderPool);

public:

	typedef Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId> ProgramMap;
	struct VkShaderRuntimeInfo
	{
		CoreGraphics::ShaderFeature::Mask activeMask;
		CoreGraphics::ShaderProgramId activeShaderProgram;
		ProgramMap programMap;
	};

	/// constructor
	VkShaderPool();
	/// destructor
	virtual ~VkShaderPool();

	/// get shader-program id, which can be used to directly access a program in a shader
	CoreGraphics::ShaderProgramId GetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);

	/// create resource table 
	CoreGraphics::ResourceTableId CreateResourceTable(const CoreGraphics::ShaderId id, const IndexT group);
	/// create constant buffer from name
	CoreGraphics::ConstantBufferId CreateConstantBuffer(const CoreGraphics::ShaderId id, const Util::StringAtom& name);
	/// create constant buffer from id
	CoreGraphics::ConstantBufferId CreateConstantBuffer(const CoreGraphics::ShaderId id, const IndexT cbIndex);

	/// get constant buffer binding from name
	const CoreGraphics::ConstantBinding GetConstantBinding(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const;
	/// get constant buffer binding from index
	const CoreGraphics::ConstantBinding GetConstantBinding(const CoreGraphics::ShaderId id, const IndexT cIndex) const;
	/// get constant buffer bindings
	const SizeT GetConstantBindingsCount(const CoreGraphics::ShaderId id) const;

	/// get the resource table layout
	CoreGraphics::ResourceTableLayoutId GetResourceTableLayout(const CoreGraphics::ShaderId id, const IndexT group);
	/// get the pipeline layout
	CoreGraphics::ResourcePipelineId GetResourcePipeline(const CoreGraphics::ShaderId id);

	/// get number of variables for shader
	const SizeT GetConstantCount(const CoreGraphics::ShaderId id) const;
	/// get type of constant by index
	const CoreGraphics::ShaderConstantType GetConstantType(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get type of constant by index
	const CoreGraphics::ShaderConstantType GetConstantType(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const;
	/// get name of constant block wherein the variable resides
	const Util::StringAtom GetConstantBlockName(const CoreGraphics::ShaderId id, const Util::StringAtom& name);
	/// get name of constant block wherein the variable resides
	const Util::StringAtom GetConstantBlockName(const CoreGraphics::ShaderId id, const IndexT cIndex);
	/// get name of constant by index
	const Util::StringAtom GetConstantName(const CoreGraphics::ShaderId id, const IndexT i) const;

	/// get constant buffer group index of constant
	const IndexT GetConstantGroup(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const;
	/// get constant buffer slot index of constant
	const IndexT GetConstantSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const;

	/// get number of constant blocks
	const SizeT GetConstantBufferCount(const CoreGraphics::ShaderId id) const;
	/// get size of constant buffer
	const SizeT GetConstantBufferSize(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get name of constnat buffer
	const Util::StringAtom GetConstantBufferName(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get slot of constant buffer based on index
	const IndexT GetConstantBufferResourceSlot(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get group of constant buffer based on index
	const IndexT GetConstantBufferResourceGroup(const CoreGraphics::ShaderId id, const IndexT i) const;
	/// get slot of shader resource
	const IndexT GetResourceSlot(const CoreGraphics::ShaderId id, const Util::StringAtom& name) const;
	

	/// get list all mask-program pairs
	const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>& GetPrograms(const CoreGraphics::ShaderId id);
	/// get name of program
	Util::String GetProgramName(CoreGraphics::ShaderProgramId id);

private:
	friend class VkVertexSignaturePool;
	friend class VkPipelineDatabase;
	friend const CoreGraphics::ConstantBufferId CoreGraphics::CreateConstantBuffer(const CoreGraphics::ConstantBufferCreateInfo& info);

	friend void ::CoreGraphics::SetShaderProgram(const CoreGraphics::ShaderProgramId& pro);
	friend void ::CoreGraphics::SetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);

	/// get shader program
	AnyFX::VkProgram* GetProgram(const CoreGraphics::ShaderProgramId shaderProgramId);
	/// load shader
	LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) override;
	/// reload shader
	LoadStatus ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream) override;
	
	/// unload shader
	void Unload(const Resources::ResourceId id) override;

	typedef Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId> UniformBufferMap;
	typedef Util::Dictionary<uint32_t, Util::Array<CoreGraphics::ConstantBufferId>> UniformBufferGroupMap;

	struct DescriptorSetBinding
	{
		VkDescriptorSet set;
		VkPipelineLayout layout;
		IndexT slot;
	};

	struct BufferMapping
	{
		uint32_t index;
		uint32_t offset;
	};


	struct VkShaderSetupInfo
	{
		VkDevice dev;
		Resources::ResourceName name;
		CoreGraphics::ShaderIdentifier::Code id;
		UniformBufferMap uniformBufferMap;				// uniform buffers shared by all shader states
		UniformBufferGroupMap uniformBufferGroupMap;	// same as above but grouped

		CoreGraphics::ResourcePipelineId pipelineLayout;
		Util::FixedArray<CoreGraphics::ResourcePipelinePushConstantRange> constantRangeLayout;
		Util::Array<CoreGraphics::SamplerId> immutableSamplers;
		Util::Dictionary<Util::StringAtom, uint32_t> resourceIndexMap;
		Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBinding> constantBindings;
		Util::FixedArray<std::pair<uint32_t, CoreGraphics::ResourceTableLayoutId>> descriptorSetLayouts;
		Util::Dictionary<uint32_t, uint32_t> descriptorSetLayoutMap;
	};

	/// this member allocates shaders
	Ids::IdAllocator<
		AnyFX::ShaderEffect*,						//0 effect
		VkShaderSetupInfo,							//1 setup immutable values
		VkShaderRuntimeInfo,						//2 runtime values
		VkShaderProgramAllocator					//3 variations
	> shaderAlloc;
	__ImplementResourceAllocatorTyped(shaderAlloc, ShaderIdType);	

	//__ResourceAllocator(VkShader);
	CoreGraphics::ShaderProgramId activeShaderProgram;
	CoreGraphics::ShaderFeature::Mask activeMask;
	Util::Dictionary<Ids::Id24, Ids::Id32> slicedStateMap;
};

} // namespace Vulkan