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

namespace Vulkan
{

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

/// set int value
void SetInt(VkShaderVariableVariableBinding& bind, int value);
/// set int array values
void SetIntArray(VkShaderVariableVariableBinding& bind, const int* values, SizeT count);
/// set float value
void SetFloat(VkShaderVariableVariableBinding& bind, float value);
/// set float array values
void SetFloatArray(VkShaderVariableVariableBinding& bind, const float* values, SizeT count);
/// set vector value
void SetFloat2(VkShaderVariableVariableBinding& bind, const Math::float2& value);
/// set vector array values
void SetFloat2Array(VkShaderVariableVariableBinding& bind, const Math::float2* values, SizeT count);
/// set vector value
void SetFloat4(VkShaderVariableVariableBinding& bind, const Math::float4& value);
/// set vector array values
void SetFloat4Array(VkShaderVariableVariableBinding& bind, const Math::float4* values, SizeT count);
/// set matrix value
void SetMatrix(VkShaderVariableVariableBinding& bind, const Math::matrix44& value);
/// set matrix array values
void SetMatrixArray(VkShaderVariableVariableBinding& bind, const Math::matrix44* values, SizeT count);
/// set bool value
void SetBool(VkShaderVariableVariableBinding& bind, bool value);
/// set bool array values
void SetBoolArray(VkShaderVariableVariableBinding& bind, const bool* values, SizeT count);
/// set texture value
void SetTexture(VkShaderVariableVariableBinding& bind, VkShaderVariableResourceBinding& res, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::TextureId tex);
/// set constant buffer
void SetConstantBuffer(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::ConstantBufferId buf);
/// set shader read-write image
void SetShaderReadWriteTexture(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::ShaderRWTextureId tex);
/// set shader read-write as texture
void SetShaderReadWriteTexture(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::TextureId tex);
/// set shader read-write buffer
void SetShaderReadWriteBuffer(VkShaderVariableResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const CoreGraphics::ShaderRWBufferId buf);

/// setup from AnyFX variable
void VkShaderVariableSetup(AnyFX::VkVariable* var, Ids::Id24 id, VkShaderVariableAllocator& allocator, const VkDescriptorSet set = VK_NULL_HANDLE);
/// setup from AnyFX varbuffer
void VkShaderVariableSetup(AnyFX::VkVarbuffer* var, Ids::Id24 id, VkShaderVariableAllocator& allocator, const VkDescriptorSet set);
/// setup from AnyFX varblock
void VkShaderVariableSetup(AnyFX::VkVarblock* var, Ids::Id24 id, VkShaderVariableAllocator& allocator, const VkDescriptorSet set);

/// bind variable to uniform buffer
void VkShaderVariableBindToUniformBuffer(const CoreGraphics::ShaderVariableId var, const CoreGraphics::ConstantBufferId buffer, VkShaderVariableAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue);
/// bind variable to push constant range
void VkShaderVariableBindToPushConstantRange(const CoreGraphics::ShaderVariableId var, uint8_t* buffer, VkShaderVariableAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue);


//------------------------------------------------------------------------------
/**
*/
template<class T>
inline void
VkShaderVariableUpdatePushRange(uint8_t* buf, uint32_t size, const T& data)
{
	memcpy(buf, &data, size);
}

} // namespace Vulkan