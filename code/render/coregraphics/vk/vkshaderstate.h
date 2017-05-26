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
public:
	class VkDerivativeState;

	/// constructor
	VkShaderState();
	/// destructor
	virtual ~VkShaderState();

	/// discard the shader instance, must be called when instance no longer needed
	void Discard();

	/// begin all uniform buffers for a synchronous update
	void BeginUpdateSync();
	/// end buffer updates for all uniform buffers
	void EndUpdateSync();
	/// apply shader from which this state was created
	void Apply();
	/// commit changes before rendering
	void Commit();

	/// add descriptor set write, which will be performed on the next begin
	void AddDescriptorWrite(const VkWriteDescriptorSet& write);

	/// get uniform buffer by index
	const Ptr<CoreGraphics::ConstantBuffer>& GetConstantBuffer(IndexT i) const;
	/// get uniform buffer by name
	const Ptr<CoreGraphics::ConstantBuffer>& GetConstantBuffer(const Util::StringAtom& name) const;
	/// get number of uniform buffers
	const SizeT GetNumConstantBuffers() const;

	/// use this if some system want to allocate and use their own descriptor sets
	void SetDescriptorSet(const VkDescriptorSet& set, const IndexT slot);
	/// create new derivative state using group
	Ptr<VkDerivativeState> CreateDerivative(const IndexT group);

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
	struct BufferMapping;

	/// setup the shader instance from its original shader object
	void Setup(const Ptr<CoreGraphics::Shader>& origShader, const Util::Array<IndexT>& groups, bool createResourceSet);

	/// sets up variables
	void SetupVariables(const Util::Array<IndexT>& groups);
	/// setup uniform buffers for shader state
	void SetupUniformBuffers(const Util::Array<IndexT>& groups);

	/// update descriptor sets
	void UpdateDescriptorSets();

	/// create array of offsets
	void CreateOffsetArray(Util::Array<uint32_t>& outOffsets, const IndexT group);
	/// get index in offset array based on binding
	BufferMapping GetBufferMapping(const IndexT& group, const IndexT& binding);

	struct DeferredVariableToBufferBind
	{
		unsigned offset;
		unsigned size;
		unsigned arraySize;
	};
	typedef Util::KeyValuePair<DeferredVariableToBufferBind, Ptr<CoreGraphics::ConstantBuffer>> VariableBufferBinding;
	Util::Dictionary<Util::StringAtom, VariableBufferBinding> uniformVariableBinds;

	typedef Util::KeyValuePair<Ptr<CoreGraphics::ShaderVariable>, Ptr<CoreGraphics::ConstantBuffer>> BlockBufferBinding;
	Util::Array<BlockBufferBinding> blockToBufferBindings;

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
	Util::FixedArray<VkDescriptorSet> sets;
	Util::FixedArray<DescriptorSetBinding> setBindnings;
	Util::FixedArray<Util::Array<uint32_t>> setOffsets;
	Util::FixedArray<Util::Dictionary<uint32_t, BufferMapping>> setBufferMapping;

	Util::Array<VkWriteDescriptorSet> pendingSetWrites;
	Util::Dictionary<uint32_t, uint32_t> groupIndexMap;

	Util::Array<uint32_t> offsets;
	Util::Dictionary<Util::String, uint32_t> offsetsByName;
	Util::Dictionary<Ptr<CoreGraphics::ConstantBuffer>, uint32_t> instances;

	bool setsDirty;
	uint8_t* pushData;
	uint32_t pushSize;
	VkPipelineLayout pushLayout;

	AnyFX::ShaderEffect* effect;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ConstantBuffer>&
VkShaderState::GetConstantBuffer(IndexT i) const
{
	return this->shader->buffers.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ConstantBuffer>&
VkShaderState::GetConstantBuffer(const Util::StringAtom& name) const
{
	return this->shader->buffers[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const SizeT
VkShaderState::GetNumConstantBuffers() const
{
	return this->shader->buffers.Size();
}

} // namespace Vulkan