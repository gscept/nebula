#pragma once
//------------------------------------------------------------------------------
/**
	Interface for setting shader constants, both in constant buffers as well as 
	constant push ranges.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "lowlevel/vk/vkvarblock.h"
#include "lowlevel/vk/vkvarbuffer.h"
#include "lowlevel/vk/vkvariable.h"
#include "resources/resourcepool.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/resourcetable.h"

namespace Vulkan
{

struct VkShaderConstantMemoryBinding
{
	bool isbuffer;
	uint32_t offset;
	uint32_t size;
	bool isvalid;
	int8_t* defaultValue;
	union
	{
		uint8_t* push;									// push range backing
		Ids::Id32 uniformBuffer;						// uniform buffer backing
														//CoreGraphics::ConstantBufferId uniformBuffer;	// uniform buffer backing
	} backing;
};

union DescriptorWrite
{
	VkDescriptorImageInfo img;
	VkDescriptorBufferInfo buf;
	VkBufferView texBuf;
};

struct VkShaderConstantDescriptorBinding
{
	bool dynamicOffset;
	CoreGraphics::ResourceTableId set;
	uint32_t setBinding;
	bool textureIsHandle;
	DescriptorWrite write;
};

struct VkShaderConstantSetupInfo
{
	Util::StringAtom name;
	CoreGraphics::ShaderConstantType type;
};

typedef Ids::IdAllocator<
	VkShaderConstantMemoryBinding,			//0 either push range or buffer
	VkShaderConstantDescriptorBinding,			//1 if variable is resource (constant buffer, sampler state, sampler+texture, image)
	VkShaderConstantSetupInfo					//2 name of variable
> VkShaderConstantAllocator;

/// set int value
void SetInt(VkShaderConstantMemoryBinding& bind, int value);
/// set int array values
void SetIntArray(VkShaderConstantMemoryBinding& bind, const int* values, SizeT count);
/// set int value
void SetUInt(VkShaderConstantMemoryBinding& bind, uint value);
/// set int array values
void SetUIntArray(VkShaderConstantMemoryBinding& bind, const uint* values, SizeT count);
/// set float value
void SetFloat(VkShaderConstantMemoryBinding& bind, float value);
/// set float array values
void SetFloatArray(VkShaderConstantMemoryBinding& bind, const float* values, SizeT count);
/// set vector value
void SetFloat2(VkShaderConstantMemoryBinding& bind, const Math::float2& value);
/// set vector array values
void SetFloat2Array(VkShaderConstantMemoryBinding& bind, const Math::float2* values, SizeT count);
/// set vector value
void SetFloat4(VkShaderConstantMemoryBinding& bind, const Math::float4& value);
/// set vector array values
void SetFloat4Array(VkShaderConstantMemoryBinding& bind, const Math::float4* values, SizeT count);
/// set matrix value
void SetMatrix(VkShaderConstantMemoryBinding& bind, const Math::matrix44& value);
/// set matrix array values
void SetMatrixArray(VkShaderConstantMemoryBinding& bind, const Math::matrix44* values, SizeT count);
/// set bool value
void SetBool(VkShaderConstantMemoryBinding& bind, bool value);
/// set bool array values
void SetBoolArray(VkShaderConstantMemoryBinding& bind, const bool* values, SizeT count);
/// set texture value
void SetTexture(VkShaderConstantMemoryBinding& bind, VkShaderConstantDescriptorBinding& res, const CoreGraphics::TextureId tex);
/// set render texture value
void SetRenderTexture(VkShaderConstantMemoryBinding& bind, VkShaderConstantDescriptorBinding& res, const CoreGraphics::RenderTextureId tex);
/// set constant buffer
void SetConstantBuffer(VkShaderConstantDescriptorBinding& bind, const CoreGraphics::ConstantBufferId buf);
/// set shader read-write image
void SetShaderReadWriteTexture(VkShaderConstantMemoryBinding& bind, VkShaderConstantDescriptorBinding& res, const CoreGraphics::ShaderRWTextureId tex);
/// set shader read-write as texture
void SetShaderReadWriteTexture(VkShaderConstantDescriptorBinding& bind, const CoreGraphics::TextureId tex);
/// set shader read-write buffer
void SetShaderReadWriteBuffer(VkShaderConstantDescriptorBinding& bind, const CoreGraphics::ShaderRWBufferId buf);

/// setup from AnyFX variable
void VkShaderConstantSetup(AnyFX::VkVariable* var, Ids::Id24 id, VkShaderConstantAllocator& allocator, const CoreGraphics::ResourceTableId& tid);
/// setup from AnyFX varbuffer
void VkShaderConstantSetup(AnyFX::VkVarbuffer* var, Ids::Id24 id, VkShaderConstantAllocator& allocator, const CoreGraphics::ResourceTableId& tid);
/// setup from AnyFX varblock
void VkShaderConstantSetup(AnyFX::VkVarblock* var, Ids::Id24 id, VkShaderConstantAllocator& allocator, const CoreGraphics::ResourceTableId& tid);

/// bind variable to uniform buffer
void VkShaderConstantBindToUniformBuffer(const CoreGraphics::ShaderConstantId var, const CoreGraphics::ConstantBufferId buffer, VkShaderConstantAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue);
/// bind variable to push constant range
void VkShaderConstantBindToPushConstantRange(const CoreGraphics::ShaderConstantId var, uint8_t* buffer, VkShaderConstantAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue);


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