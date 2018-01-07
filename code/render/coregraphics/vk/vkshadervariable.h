#pragma once
//------------------------------------------------------------------------------
/**
	Interface for setting shader variables (in constant buffers)
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "lowlevel/vk/vkvarblock.h"
#include "lowlevel/vk/vkvarbuffer.h"
#include "lowlevel/vk/vkvariable.h"
#include "resources/resourcepool.h"
#include "coregraphics/constantbuffer.h"

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
class VkShaderVariable
{
	union VkShaderVariableVariableBinding;
	struct VkShaderVariableResourceBinding;
	struct PushRangeBinding;
public:

	/// set int value
	static void SetInt(VkShaderVariableVariableBinding& bind, int value);
	/// set int array values
	static void SetIntArray(VkShaderVariableVariableBinding& bind, const int* values, SizeT count);
	/// set float value
	static void SetFloat(VkShaderVariableVariableBinding& bind, float value);
	/// set float array values
	static void SetFloatArray(VkShaderVariableVariableBinding& bind, const float* values, SizeT count);
	/// set vector value
	static void SetFloat2(VkShaderVariableVariableBinding& bind, const Math::float2& value);
	/// set vector array values
	static void SetFloat2Array(VkShaderVariableVariableBinding& bind, const Math::float2* values, SizeT count);
	/// set vector value
	static void SetFloat4(VkShaderVariableVariableBinding& bind, const Math::float4& value);
	/// set vector array values
	static void SetFloat4Array(VkShaderVariableVariableBinding& bind, const Math::float4* values, SizeT count);
	/// set matrix value
	static void SetMatrix(VkShaderVariableVariableBinding& bind, const Math::matrix44& value);
	/// set matrix array values
	static void SetMatrixArray(VkShaderVariableVariableBinding& bind, const Math::matrix44* values, SizeT count);
	/// set bool value
	static void SetBool(VkShaderVariableVariableBinding& bind, bool value);
	/// set bool array values
	static void SetBoolArray(VkShaderVariableVariableBinding& bind, const bool* values, SizeT count);
	/// set texture value
	static void SetTexture(VkShaderVariableVariableBinding& bind, VkShaderVariableResourceBinding& res, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::TextureId tex);
	/// set constant buffer
	static void SetConstantBuffer(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::ConstantBufferId buf);
	/// set shader read-write image
	static void SetShaderReadWriteTexture(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::ShaderRWTextureId tex);
	/// set shader read-write as texture
	static void SetShaderReadWriteTexture(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::TextureId tex);
	/// set shader read-write buffer
	static void SetShaderReadWriteBuffer(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::ShaderRWBufferId buf);


private:

	friend class Vulkan::VkShaderPool;
	friend class Vulkan::VkShaderState;
	friend class Vulkan::VkShaderServer;
	friend class Vulkan::VkUniformBuffer;
	friend class Vulkan::VkPass;
	friend class Lighting::VkLightServer;

	/// update push constant buffer, the variable already knows the offset
	template<class T> static void UpdatePushRange(uint8_t* buf, uint32_t size, const T& data);

	union VkShaderVariableVariableBinding
	{
		bool isbuffer;
		uint32_t offset;
		uint32_t size;
		bool isvalid;
		int8_t* defaultValue;
		union
		{
			uint8_t* push;									// push range backing
			Ids::Id24 uniformBuffer : 24;					// uniform buffer backing
			//CoreGraphics::ConstantBufferId uniformBuffer;	// uniform buffer backing
		} backing;
	};

	union DescriptorWrite
	{
		VkDescriptorImageInfo img;
		VkDescriptorBufferInfo buf;
		VkBufferView texBuf;
	};

	struct VkShaderVariableResourceBinding
	{
		bool dynamicOffset;
		VkDescriptorSet set;
		uint32_t setBinding;
		bool textureIsHandle;
		DescriptorWrite write;
	};

	struct VkShaderVariableSetupInfo
	{
		Util::StringAtom name;
		CoreGraphics::ShaderVariableType type;
	};

	typedef Ids::IdAllocator<
		VkShaderVariableVariableBinding,			//0 either push range or buffer
		VkShaderVariableResourceBinding,			//1 if variable is resource (constant buffer, sampler state, sampler+texture, image)
		VkShaderVariableSetupInfo					//2 name of variable
	> VkShaderVariableAllocator;

	/// setup from AnyFX variable
	static void Setup(AnyFX::VkVariable* var, Ids::Id24 id, VkShaderVariableAllocator& allocator, VkDescriptorSet& set);
	/// setup from AnyFX varbuffer
	static void Setup(AnyFX::VkVarbuffer* var, Ids::Id24 id, VkShaderVariableAllocator& allocator, VkDescriptorSet& set);
	/// setup from AnyFX varblock
	static void Setup(AnyFX::VkVarblock* var, Ids::Id24 id, VkShaderVariableAllocator& allocator, VkDescriptorSet& set);

	/// bind variable to uniform buffer
	static void BindToUniformBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ConstantBufferId buffer, VkShaderVariableAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue);
	/// bind variable to push constant range
	static void BindToPushConstantRange(const CoreGraphics::ShaderVariableId var, uint8_t* buffer, VkShaderVariableAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue);
};

//------------------------------------------------------------------------------
/**
*/
template<class T>
inline void
VkShaderVariable::UpdatePushRange(uint8_t* buf, uint32_t size, const T& data)
{
	memcpy(buf, &data, size);
}

} // namespace Vulkan