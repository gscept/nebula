#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader instance (local variables and such) in Vulkan.

	To get the shader state to be modified, one has to modify the value of a shader variable.
	To get the shader state to apply a specific descriptor set (0 is enabled by default), 
	one has to explain that the shader state should commit that descriptor set.

	The VkShaderState can create a derivative state, which is a derivation of a single
	group (descriptor set) and a set of offsets which are not stored in the set. The way
	to use it is like this:
	1. Create derivative set.
	2. Set offset pointer.
	3. Apply derivative.
		3.5 Update variables using shader state (variables retrieved from it)
	4. Commit derivative.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/shaderstatebase.h"
#include "lowlevel/afxapi.h"
namespace Lighting
{
	class VkLightServer;
}
namespace Vulkan
{

class VkShaderState : public Base::ShaderStateBase
{
	__DeclareClass(VkShaderState);
	struct SetupInfo;
	struct RuntimeInfo;
public:
	class VkDerivativeState;


	/// constructor
	VkShaderState();
	/// destructor
	virtual ~VkShaderState();

	/// discard the shader instance, must be called when instance no longer needed
	static void Discard();

	/// begin all uniform buffers for a synchronous update
	static void BeginUpdateSync();
	/// end buffer updates for all uniform buffers
	static void EndUpdateSync();
	/// apply shader from which this state was created
	static void Apply();
	/// commit changes before rendering
	static void Commit(Ids::Id24 currentProgram, Util::Array<VkWriteDescriptorSet>& writes, bool& setsDirty, VkShaderState::RuntimeInfo& stateInfo);

	/// use this if some system want to allocate and use their own descriptor sets
	static void SetDescriptorSet(const VkDescriptorSet& set, const IndexT slot);
	/// create new derivative state using group
	static Ptr<VkDerivativeState> CreateDerivative(const IndexT group);

	class VkDerivativeState : public Core::RefCounted
	{
		__DeclareClass(VkDerivativeState);
	
		friend class VkShaderState;
		VkDescriptorSet set;
		uint32_t group;
		VkPipelineLayout layout;
		VkShaderState* parent;

	public:
		Util::Array<Ptr<CoreGraphics::ConstantBuffer>> buffers;
		VkPipelineBindPoint bindPoint;
		uint32_t offsetCount;
		uint32_t* offsets;
		bool bindShared;

		/// applies derivative, all shader state variable changes will be using said derivative
		void Apply();
		/// commit derivative 
		void Commit();
		/// resets derivative set, effectively counteracting Apply
		void Reset();
	};
private:
	friend class Base::ShaderBase;
	friend class VkShader;
	friend class Lighting::VkLightServer;
	friend class VkRenderDevice;
	friend class VkShaderPool;
	struct BufferMapping;

	/// update descriptor sets
	static void UpdateDescriptorSets(Util::Array<VkWriteDescriptorSet>& writes, bool& dirty);

	/// create array of offsets
	static void CreateOffsetArray(Util::Array<uint32_t>& outOffsets, const IndexT group);
	/// get index in offset array based on binding
	static BufferMapping GetBufferMapping(const IndexT& group, const IndexT& binding);

#pragma pack(push, 16)
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
#pragma pack(pop)

	struct RuntimeInfo
	{
		Util::FixedArray<DescriptorSetBinding> setBindings;
		Util::FixedArray<Util::Array<uint32_t>> setOffsets;
		VkPipelineLayout pushLayout;
		uint8_t* pushData;
		uint32_t pushDataSize;
		bool setsDirty;
		bool shared;
	};

	struct SetupInfo
	{
		Util::FixedArray<VkDescriptorSet> sets;
		Util::FixedArray<Util::Dictionary<uint32_t, BufferMapping>> setBufferMapping;
		Util::Dictionary<uint32_t, uint32_t> groupIndexMap;
		Util::Array<uint32_t> offsets;
		Util::Dictionary<Util::StringAtom, uint32_t> offsetsByName;
		Util::Dictionary<CoreGraphics::ConstantBufferId, uint32_t> instances;
		Util::Dictionary<Util::StringAtom, Ids::Id24> variableMap;
		VkPipelineLayout pipelineLayout;
	};

	typedef Ids::IdAllocator<
		AnyFX::ShaderEffect*,												//0 effect
		SetupInfo,															//1 setup info
		RuntimeInfo,														//2 runtime info
		VkShaderVariable::ShaderVariableAllocator,							//3 variable allocator
		Util::Array<VkWriteDescriptorSet>									//4 descriptor set writes
	> ShaderStateAllocator;

	/// setup the shader instance from its original shader object
	static void Setup(
		const Ids::Id24 id, 
		AnyFX::ShaderEffect* effect, 
		const Util::Array<IndexT>& groups,
		ShaderStateAllocator& allocator, 
		Util::FixedArray<VkDescriptorSet>& sets,
		Util::FixedArray<VkDescriptorSetLayout>& setLayouts,
		bool createUniqueSet);

	/// sets up variables
	static void SetupVariables(AnyFX::ShaderEffect* effect, RuntimeInfo& runtime, SetupInfo& setup, VkShaderVariable::ShaderVariableAllocator& varAllocator, const Util::Array<IndexT>& groups);
	/// setup uniform buffers for shader state
	static void SetupUniformBuffers(AnyFX::ShaderEffect* effect, RuntimeInfo& runtime, SetupInfo& setup, VkShaderVariable::ShaderVariableAllocator& varAllocator, const Util::Array<IndexT>& groups);
};

} // namespace Vulkan