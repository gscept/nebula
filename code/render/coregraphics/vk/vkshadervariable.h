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
class VkStreamShaderLoader;
class VkShaderState;
class VkShader;
class VkShaderServer;
class VkUniformBuffer;
class VkPass;
class VkShaderVariable : public Base::ShaderVariableBase
{
	__DeclareClass(VkShaderVariable);
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
	void SetInt(int value);
	/// set int array values
	void SetIntArray(const int* values, SizeT count);
	/// set float value
	void SetFloat(float value);
	/// set float array values
	void SetFloatArray(const float* values, SizeT count);
	/// set vector value
	void SetFloat2(const Math::float2& value);
	/// set vector array values
	void SetFloat2Array(const Math::float2* values, SizeT count);
	/// set vector value
	void SetFloat4(const Math::float4& value);
	/// set vector array values
	void SetFloat4Array(const Math::float4* values, SizeT count);
	/// set matrix value
	void SetMatrix(const Math::matrix44& value);
	/// set matrix array values
	void SetMatrixArray(const Math::matrix44* values, SizeT count);
	/// set bool value
	void SetBool(bool value);
	/// set bool array values
	void SetBoolArray(const bool* values, SizeT count);
	/// set texture value
	void SetTexture(const Ptr<CoreGraphics::Texture>& tex);
	/// set constant buffer
	void SetConstantBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buf);
	/// set shader read-write image
	void SetShaderReadWriteTexture(const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex);	
	/// set shader read-write as texture
	void SetShaderReadWriteTexture(const Ptr<CoreGraphics::Texture>& tex);
	/// set shader read-write buffer
	void SetShaderReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf);

	/// returns true if shader variable has an offset into any uniform buffer for the shader state
	const bool IsActive() const;

private:

	friend class Vulkan::VkStreamShaderLoader;
	friend class Vulkan::VkShaderState;
	friend class Vulkan::VkShaderServer;
	friend class Vulkan::VkUniformBuffer;
	friend class Vulkan::VkPass;
	friend class Lighting::VkLightServer;

	/// setup from AnyFX variable
	void Setup(AnyFX::VkVariable* var, const Ptr<VkShaderState>& shader, const VkDescriptorSet& set);
	/// setup from AnyFX varbuffer
	void Setup(AnyFX::VkVarbuffer* var, const Ptr<VkShaderState>& shader, const VkDescriptorSet& set);
	/// setup from AnyFX varblock
	void Setup(AnyFX::VkVarblock* var, const Ptr<VkShaderState>& shader, const VkDescriptorSet& set);

	/// update push constant buffer, the variable already knows the offset
	template<class T> void UpdatePushRange(uint32_t size, const T& data);
#pragma pack(push, 16)
	struct BufferBinding
	{
		Ptr<CoreGraphics::ConstantBuffer> uniformBuffer;
		uint32_t offset;
		uint32_t size;
		bool isvalid;
		int8_t* defaultValue;
	} bufferBinding;

	struct PushRangeBinding
	{
		uint8_t* buffer;
		uint32_t offset;
		uint32_t size;
		bool isvalid;
		int8_t* defaultValue;
	} pushRangeBinding;
#pragma pack(pop)

	union
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
};

//------------------------------------------------------------------------------
/**
*/
template<class T> void
VkShaderVariable::UpdatePushRange(uint32_t size, const T& data)
{
	n_assert(this->pushRangeBinding.isvalid);
	memcpy(this->pushRangeBinding.buffer + this->pushRangeBinding.offset, &data, size);
}

} // namespace Vulkan