#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader variable in Vulkan (push constant?!?!?!?!)
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/shadervariablebase.h"
#include "lowlevel/vk/vkvarblock.h"
#include "lowlevel/vk/vkvarbuffer.h"
#include "lowlevel/vk/vkvariable.h"
#include "resources/resourcepool.h"
#include "coregraphics/constantbuffer.h"

namespace CoreGraphics
{
class ConstantBuffer;
class Texture;
class ShaderReadWriteTexture;
class ShaderReadWriteBuffer;
}

namespace Lighting
{
class VkLightServer;
}

namespace Vulkan
{
class VkShaderPool;
class VkShaderState;
class VkShader;
class VkShaderServer;
class VkUniformBuffer;
class VkPass;
class VkShaderVariable : public Base::ShaderVariableBase
{
	__DeclareClass(VkShaderVariable);
	union VariableBinding;
	struct ResourceBinding;
	struct PushRangeBinding;
public:

	/// constructor
	VkShaderVariable();
	/// destructor
	virtual ~VkShaderVariable();

	/// bind variable to uniform buffer
	void BindToUniformBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buffer, uint32_t offset, uint32_t size, int8_t* defaultValue);
	/// bind variable to push constant range
	void BindToPushConstantRange(uint8_t* buffer, uint32_t offset, uint32_t size, int8_t* defaultValue);

	/// set int value
	static void SetInt(VariableBinding& bind, int value);
	/// set int array values
	static void SetIntArray(VariableBinding& bind, const int* values, SizeT count);
	/// set float value
	static void SetFloat(VariableBinding& bind, float value);
	/// set float array values
	static void SetFloatArray(VariableBinding& bind, const float* values, SizeT count);
	/// set vector value
	static void SetFloat2(VariableBinding& bind, const Math::float2& value);
	/// set vector array values
	static void SetFloat2Array(VariableBinding& bind, const Math::float2* values, SizeT count);
	/// set vector value
	static void SetFloat4(VariableBinding& bind, const Math::float4& value);
	/// set vector array values
	static void SetFloat4Array(VariableBinding& bind, const Math::float4* values, SizeT count);
	/// set matrix value
	static void SetMatrix(VariableBinding& bind, const Math::matrix44& value);
	/// set matrix array values
	static void SetMatrixArray(VariableBinding& bind, const Math::matrix44* values, SizeT count);
	/// set bool value
	static void SetBool(VariableBinding& bind, bool value);
	/// set bool array values
	static void SetBoolArray(VariableBinding& bind, const bool* values, SizeT count);
	/// set texture value
	static void SetTexture(VariableBinding& bind, ResourceBinding& res, Util::Array<VkWriteDescriptorSet>& writes, const Resources::ResourceId tex);
	/// set constant buffer
	static void SetConstantBuffer(ResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ConstantBuffer>& buf);
	/// set shader read-write image
	static void SetShaderReadWriteTexture(ResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex);
	/// set shader read-write as texture
	static void SetShaderReadWriteTexture(ResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::Texture>& tex);
	/// set shader read-write buffer
	static void SetShaderReadWriteBuffer(ResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf);

	/// returns true if shader variable has an offset into any uniform buffer for the shader state
	const bool IsActive() const;

private:

	friend class Vulkan::VkShaderPool;
	friend class Vulkan::VkShaderState;
	friend class Vulkan::VkShaderServer;
	friend class Vulkan::VkUniformBuffer;
	friend class Vulkan::VkPass;
	friend class Lighting::VkLightServer;

	/// update push constant buffer, the variable already knows the offset
	template<class T> static void UpdatePushRange(PushRangeBinding& pushRangeBinding, uint32_t size, const T& data);
#pragma pack(push, 16)
	struct BufferBinding
	{
		ConstantBufferId uniformBuffer;
		uint32_t offset;
		uint32_t size;
		bool isvalid;
		int8_t* defaultValue;
	};

	struct PushRangeBinding
	{
		uint8_t* buffer;
		uint32_t offset;
		uint32_t size;
		bool isvalid;
		int8_t* defaultValue;
	};
#pragma pack(pop)

	union VariableBinding
	{
		bool isbuffer;
		BufferBinding buf;
		PushRangeBinding push;
	};

	union DescriptorWrite
	{
		VkDescriptorImageInfo img;
		VkDescriptorBufferInfo buf;
		VkBufferView texBuf;
	};

	bool textureHandle;
	bool dynamicOffset;
	bool active;
	uint32_t binding;
	Ptr<VkShaderState> shader;
	VkDescriptorSet set;

	struct ResourceBinding
	{
		bool dynamicOffset;
		VkDescriptorSet set;
		uint32_t setBinding;
		bool textureIsHandle;
		DescriptorWrite write;
	};

	struct SetupInfo
	{
		Util::StringAtom name;
		ShaderVariableBase::Type type;
	};

	typedef Ids::IdAllocator<
		VariableBinding,			//0 either push range or buffer
		ResourceBinding,			//1 if variable is resource (constant buffer, sampler state, sampler+texture, image)
		SetupInfo					//2 name of variable
	> ShaderVariableAllocator;

	/// setup from AnyFX variable
	static void Setup(AnyFX::VkVariable* var, Ids::Id24 id, ShaderVariableAllocator& allocator, VkDescriptorSet& set);
	/// setup from AnyFX varbuffer
	static void Setup(AnyFX::VkVarbuffer* var, Ids::Id24 id, ShaderVariableAllocator& allocator, VkDescriptorSet& set);
	/// setup from AnyFX varblock
	static void Setup(AnyFX::VkVarblock* var, Ids::Id24 id, ShaderVariableAllocator& allocator, VkDescriptorSet& set);
};

//------------------------------------------------------------------------------
/**
*/
template<class T> void
VkShaderVariable::UpdatePushRange(PushRangeBinding& pushRangeBinding, uint32_t size, const T& data)
{
	memcpy(pushRangeBinding.buffer + pushRangeBinding.offset, &data, size);
}

} // namespace Vulkan