//------------------------------------------------------------------------------
// vkshadervariable.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshadervariable.h"
#include "vkshaderstate.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include "coregraphics/texture.h"

namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderVariable, 'VKSV', Base::ShaderVariableBase);
//------------------------------------------------------------------------------
/**
*/
VkShaderVariable::VkShaderVariable() :
	bufferBinding({nullptr, 0, 0, false, nullptr}),
    pushRangeBinding({ nullptr, 0, 0, false, nullptr }),
	dynamicOffset(false),
	textureHandle(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkShaderVariable::~VkShaderVariable()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::BindToUniformBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buffer, uint32_t offset, uint32_t size, int8_t* defaultValue)
{
	this->bufferBinding.uniformBuffer = buffer;
	this->bufferBinding.offset = offset;
	this->bufferBinding.size = size;
	this->bufferBinding.defaultValue = defaultValue;
	this->bufferBinding.isvalid = true;

	// make sure that the buffer is updated (data is array since we have a char*)
	buffer->UpdateArray(defaultValue, offset, size, 1);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::BindToPushConstantRange(uint8_t* buffer, uint32_t offset, uint32_t size, int8_t* defaultValue)
{
	this->pushRangeBinding.buffer = buffer;
	this->pushRangeBinding.offset = offset;
	this->pushRangeBinding.size = size;
	this->pushRangeBinding.defaultValue = defaultValue;
	this->pushRangeBinding.isvalid = true;
	
	// copy data to buffer
	memcpy(buffer + offset, defaultValue, size);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::Setup(AnyFX::VkVariable* var, const Ptr<VkShaderState>& shader, const VkDescriptorSet& set)
{
	n_assert(0 != var);

	Util::String name = var->name.c_str();
	this->SetName(name);
	this->shader = shader;
	this->set = set;
	switch (var->type)
	{
	case AnyFX::Double:
	case AnyFX::Float:
		this->SetType(FloatType);
		break;
	case AnyFX::Short:
	case AnyFX::Integer:
	case AnyFX::UInteger:
		this->SetType(IntType);
		break;
	case AnyFX::Bool:
		this->SetType(BoolType);
		break;
	case AnyFX::Float3:
	case AnyFX::Float4:
	case AnyFX::Double3:
	case AnyFX::Double4:
	case AnyFX::Integer3:
	case AnyFX::Integer4:
	case AnyFX::UInteger3:
	case AnyFX::UInteger4:
	case AnyFX::Short3:
	case AnyFX::Short4:
	case AnyFX::Bool3:
	case AnyFX::Bool4:
		this->SetType(VectorType);
		break;
	case AnyFX::Float2:
	case AnyFX::Double2:
	case AnyFX::Integer2:
	case AnyFX::UInteger2:
	case AnyFX::Short2:
	case AnyFX::Bool2:
		this->SetType(Vector2Type);
		break;
	case AnyFX::Matrix2x2:
	case AnyFX::Matrix2x3:
	case AnyFX::Matrix2x4:
	case AnyFX::Matrix3x2:
	case AnyFX::Matrix3x3:
	case AnyFX::Matrix3x4:
	case AnyFX::Matrix4x2:
	case AnyFX::Matrix4x3:
	case AnyFX::Matrix4x4:
		this->SetType(MatrixType);
		break;
	case AnyFX::Image1D:
	case AnyFX::Image1DArray:
	case AnyFX::Image2D:
	case AnyFX::Image2DArray:
	case AnyFX::Image2DMS:
	case AnyFX::Image2DMSArray:
	case AnyFX::Image3D:
	case AnyFX::ImageCube:
	case AnyFX::ImageCubeArray:
		this->SetType(ImageReadWriteType);
		this->binding = var->bindingLayout.binding;
		break;
	case AnyFX::Sampler1D:
	case AnyFX::Sampler1DArray:
	case AnyFX::Sampler2D:
	case AnyFX::Sampler2DArray:
	case AnyFX::Sampler2DMS:
	case AnyFX::Sampler2DMSArray:
	case AnyFX::Sampler3D:
	case AnyFX::SamplerCube:
	case AnyFX::SamplerCubeArray:
		this->SetType(SamplerType);
		this->binding = var->bindingLayout.binding;
		break;
	case AnyFX::Texture1D:
	case AnyFX::Texture1DArray:
	case AnyFX::Texture2D:
	case AnyFX::Texture2DArray:
	case AnyFX::Texture2DMS:
	case AnyFX::Texture2DMSArray:
	case AnyFX::Texture3D:
	case AnyFX::TextureCube:
	case AnyFX::TextureCubeArray:
		this->SetType(TextureType);
		this->binding = var->bindingLayout.binding;
		break;
	case AnyFX::TextureHandle:
		this->SetType(TextureType);
		this->textureHandle = true;
		break;
	case AnyFX::ImageHandle:
		this->SetType(ImageReadWriteType);
		break;
	case AnyFX::SamplerHandle:
		this->SetType(SamplerType);
		this->textureHandle = true;
		break;
	default:
		this->SetType(ShaderVariableBase::ConstantBufferType);
		this->binding = var->bindingLayout.binding;
		break;
	}

}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::Setup(AnyFX::VkVarbuffer* var, const Ptr<VkShaderState>& shader, const VkDescriptorSet& set)
{
	n_assert(0 != var);
	Util::String name = var->name.c_str();
	this->SetName(name);
	this->dynamicOffset = var->Flag("DynamicOffset");
	this->binding = var->bindingLayout.binding;
	this->shader = shader;
	this->set = set;
	this->SetType(BufferReadWriteType);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::Setup(AnyFX::VkVarblock* var, const Ptr<VkShaderState>& shader, const VkDescriptorSet& set)
{
	n_assert(0 != var);
	Util::String name = var->name.c_str();
	this->SetName(name);
	this->dynamicOffset = var->Flag("DynamicOffset");
	this->binding = var->bindingLayout.binding;
	this->shader = shader;
	this->set = set;
	this->SetType(ConstantBufferType);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetInt(int value)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->Update(value, this->bufferBinding.offset, sizeof(int));
	else
		this->UpdatePushRange(sizeof(int), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetIntArray(const int* values, SizeT count)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->UpdateArray(values, this->bufferBinding.offset, sizeof(int), count);
	else
		this->UpdatePushRange(sizeof(int) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat(float value)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->Update(value, this->bufferBinding.offset, sizeof(float));
	else
		this->UpdatePushRange(sizeof(float), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloatArray(const float* values, SizeT count)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->UpdateArray(values, this->bufferBinding.offset, sizeof(float), count);
	else
		this->UpdatePushRange(sizeof(float) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat2(const Math::float2& value)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->Update(value, this->bufferBinding.offset, sizeof(Math::float2));
	else
		this->UpdatePushRange(sizeof(Math::float2), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat2Array(const Math::float2* values, SizeT count)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->UpdateArray(values, this->bufferBinding.offset, sizeof(Math::float2), count);
	else
		this->UpdatePushRange(sizeof(Math::float2) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat4(const Math::float4& value)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->Update(value, this->bufferBinding.offset, sizeof(Math::float4));
	else
		this->UpdatePushRange(sizeof(Math::float4), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat4Array(const Math::float4* values, SizeT count)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->UpdateArray(values, this->bufferBinding.offset, sizeof(Math::float4), count);
	else
		this->UpdatePushRange(sizeof(Math::float4) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetMatrix(const Math::matrix44& value)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->Update(value, this->bufferBinding.offset, sizeof(Math::matrix44));
	else
		this->UpdatePushRange(sizeof(Math::matrix44), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetMatrixArray(const Math::matrix44* values, SizeT count)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->UpdateArray(values, this->bufferBinding.offset, sizeof(Math::matrix44), count);
	else
		this->UpdatePushRange(sizeof(Math::matrix44) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetBool(bool value)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->Update(value, this->bufferBinding.offset, sizeof(bool));
	else
		this->UpdatePushRange(sizeof(bool), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetBoolArray(const bool* values, SizeT count)
{
	n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
	if (this->bufferBinding.isvalid)
		this->bufferBinding.uniformBuffer->UpdateArray(values, this->bufferBinding.offset, sizeof(bool), count);
	else
		this->UpdatePushRange(sizeof(bool) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetTexture(const Ptr<CoreGraphics::Texture>& tex)
{
	if (!tex.isvalid()) return;
	if (tex->GetVkImageView() != this->img.imageView)
	{
		if (this->textureHandle)
		{
			// update texture id
			n_assert(this->bufferBinding.isvalid || this->pushRangeBinding.isvalid);
			if (this->bufferBinding.isvalid)
				this->bufferBinding.uniformBuffer->Update(tex->GetVkId(), this->bufferBinding.offset, sizeof(uint32_t));
			else
				this->UpdatePushRange(sizeof(uint32_t), tex->GetVkId());

			//this->img.imageView = tex->GetVkImageView();
		}
		else
		{
			// dependent on type of variable, select if sampler should be coupled with sampler or if it will be assembled in-shader
			const VkImageView& view = tex->GetVkImageView();
			VkDescriptorType type;
			if (this->type == TextureType)		type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			else if (this->type == SamplerType) type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			else
			{
				n_error("Variable '%s' is must be either Texture or Integer to be assigned a texture!\n", this->name.Value());
			}

			VkWriteDescriptorSet set;
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.pNext = NULL;
			set.descriptorCount = 1;
			set.descriptorType = type;
			set.dstArrayElement = 0;
			set.dstBinding = this->binding;
			set.dstSet = this->set;
			set.pBufferInfo = NULL;
			set.pTexelBufferView = NULL;
			set.pImageInfo = &this->img;
			this->img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			this->img.imageView = tex->GetVkImageView();
			this->img.sampler = VK_NULL_HANDLE;

			// add to shader to update on next update
			this->shader->AddDescriptorWrite(set);
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetConstantBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buf)
{
	n_assert(this->type == ConstantBufferType);
	if (!buf.isvalid()) return;
	if (buf->GetVkBuffer() != this->buf.buffer)
	{
		VkWriteDescriptorSet set;
		set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set.pNext = NULL;
		set.descriptorCount = 1;
		if (this->dynamicOffset)	set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		else						set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		set.dstArrayElement = 0;
		set.dstBinding = this->binding;
		set.dstSet = this->set;
		set.pBufferInfo = &this->buf;
		set.pTexelBufferView = NULL;
		set.pImageInfo = NULL;
		this->buf.buffer = buf->GetVkBuffer();
		this->buf.offset = 0;
		this->buf.range = VK_WHOLE_SIZE;

		// add to shader to update on next update
		this->shader->AddDescriptorWrite(set);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetShaderReadWriteTexture(const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex)
{
	n_assert(this->type == ImageReadWriteType);
	if (!tex.isvalid()) return;
	if (tex->GetVkImageView() != this->img.imageView)
	{
		VkWriteDescriptorSet set;
		set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set.pNext = NULL;

		set.descriptorCount = 1;
		set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		set.dstArrayElement = 0;
		set.dstBinding = this->binding;
		set.dstSet = this->set;
		set.pBufferInfo = NULL;
		set.pTexelBufferView = NULL;
		set.pImageInfo = &this->img;
		this->img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		this->img.imageView = tex->GetVkImageView();
		this->img.sampler = VK_NULL_HANDLE;

		// add to shader to update on next update
		this->shader->AddDescriptorWrite(set);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetShaderReadWriteTexture(const Ptr<CoreGraphics::Texture>& tex)
{
	n_assert(this->type == ImageReadWriteType);
	if (!tex.isvalid()) return;
	if (tex->GetVkImageView() != this->img.imageView)
	{
		VkWriteDescriptorSet set;
		set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set.pNext = NULL;

		set.descriptorCount = 1;
		set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		set.dstArrayElement = 0;
		set.dstBinding = this->binding;
		set.dstSet = this->set;
		set.pBufferInfo = NULL;
		set.pTexelBufferView = NULL;
		set.pImageInfo = &this->img;
		this->img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		this->img.imageView = tex->GetVkImageView();
		this->img.sampler = VK_NULL_HANDLE;

		// add to shader to update on next update
		this->shader->AddDescriptorWrite(set);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetShaderReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf)
{
	n_assert(this->type == BufferReadWriteType);
	if (!buf.isvalid()) return;
	if (buf->GetVkBuffer() != this->buf.buffer)
	{
		VkWriteDescriptorSet set;
		set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set.pNext = NULL;

		set.descriptorCount = 1;
		if (this->dynamicOffset) set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		else									 set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		set.dstArrayElement = 0;
		set.dstBinding = this->binding;
		set.dstSet = this->set;
		set.pBufferInfo = &this->buf;
		set.pTexelBufferView = NULL;
		set.pImageInfo = NULL;

		this->buf.buffer = buf->GetVkBuffer();
		this->buf.offset = 0;
		this->buf.range = VK_WHOLE_SIZE;

		// add to shader to update on next update
		this->shader->AddDescriptorWrite(set);
	}
}

//------------------------------------------------------------------------------
/**
*/
const bool
VkShaderVariable::IsActive() const
{
	if (this->type >= TextureType && this->type <= BufferReadWriteType) return true;
	else																return this->bufferBinding.isvalid;
}


} // namespace Vulkan
