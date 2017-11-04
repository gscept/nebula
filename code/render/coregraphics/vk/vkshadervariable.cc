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
	// empty
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
VkShaderVariable::Setup(AnyFX::VkVariable* var, Ids::Id24 id, ShaderVariableAllocator& allocator, VkDescriptorSet& set)
{
	n_assert(0 != var);

	Util::String name = var->name.c_str();

	VariableBinding& varBind = allocator.Get<0>(id);
	ResourceBinding& resBind = allocator.Get<1>(id);
	SetupInfo& setupInfo = allocator.Get<2>(id);
	
	setupInfo.name = name;
	resBind.set = set;
	switch (var->type)
	{
	case AnyFX::Double:
	case AnyFX::Float:
		setupInfo.type = FloatType;
		break;
	case AnyFX::Short:
	case AnyFX::Integer:
	case AnyFX::UInteger:
		setupInfo.type = IntType;
		break;
	case AnyFX::Bool:
		setupInfo.type = BoolType;
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
		setupInfo.type = VectorType;
		break;
	case AnyFX::Float2:
	case AnyFX::Double2:
	case AnyFX::Integer2:
	case AnyFX::UInteger2:
	case AnyFX::Short2:
	case AnyFX::Bool2:
		setupInfo.type = Vector2Type;
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
		setupInfo.type = MatrixType;
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
		setupInfo.type = ImageReadWriteType;
		resBind.setBinding = var->bindingLayout.binding;
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
		setupInfo.type = SamplerType;
		resBind.setBinding = var->bindingLayout.binding;
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
		setupInfo.type = TextureType;
		resBind.setBinding = var->bindingLayout.binding;
		break;
	case AnyFX::TextureHandle:
		setupInfo.type = TextureType;
		resBind.textureIsHandle = true;
		break;
	case AnyFX::ImageHandle:
		setupInfo.type = ImageReadWriteType;
		break;
	case AnyFX::SamplerHandle:
		setupInfo.type = SamplerType;
		resBind.textureIsHandle = true;
		break;
	default:
		setupInfo.type = ConstantBufferType;
		resBind.setBinding = var->bindingLayout.binding;
		break;
	}

}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::Setup(AnyFX::VkVarbuffer* var, Ids::Id24 id, ShaderVariableAllocator& allocator, VkDescriptorSet& set)
{
	n_assert(0 != var);
	Util::String name = var->name.c_str();
	VariableBinding& varBind = allocator.Get<0>(id);
	ResourceBinding& resBind = allocator.Get<1>(id);
	SetupInfo& setupInfo = allocator.Get<2>(id);

	setupInfo.name = name;
	setupInfo.type = BufferReadWriteType;
	resBind.dynamicOffset = var->Flag("DynamicOffset");
	resBind.setBinding = var->bindingLayout.binding;
	resBind.set = set;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::Setup(AnyFX::VkVarblock* var, Ids::Id24 id, ShaderVariableAllocator& allocator, VkDescriptorSet& set)
{
	n_assert(0 != var);
	Util::String name = var->name.c_str();
	VariableBinding& varBind = allocator.Get<0>(id);
	ResourceBinding& resBind = allocator.Get<1>(id);
	SetupInfo& setupInfo = allocator.Get<2>(id);
	
	setupInfo.name = name;
	setupInfo.type = BufferReadWriteType;
	resBind.dynamicOffset = var->Flag("DynamicOffset");
	resBind.setBinding = var->bindingLayout.binding;
	resBind.set = set;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetInt(VariableBinding& bind, int value)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->Update(value, bind.buf.offset, sizeof(int));
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(int), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetIntArray(VariableBinding& bind, const int* values, SizeT count)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->UpdateArray(values, bind.buf.offset, sizeof(int), count);
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(int) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat(VariableBinding& bind, float value)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->Update(value, bind.buf.offset, sizeof(float));
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(float), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloatArray(VariableBinding& bind, const float* values, SizeT count)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->UpdateArray(values, bind.buf.offset, sizeof(float), count);
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(float) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat2(VariableBinding& bind, const Math::float2& value)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->Update(value, bind.buf.offset, sizeof(Math::float2));
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(Math::float2), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat2Array(VariableBinding& bind, const Math::float2* values, SizeT count)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->UpdateArray(values, bind.buf.offset, sizeof(Math::float2), count);
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(Math::float2) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat4(VariableBinding& bind, const Math::float4& value)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->Update(value, bind.buf.offset, sizeof(Math::float4));
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(Math::float4), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetFloat4Array(VariableBinding& bind, const Math::float4* values, SizeT count)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->UpdateArray(values, bind.buf.offset, sizeof(Math::float4), count);
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(Math::float4) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetMatrix(VariableBinding& bind, const Math::matrix44& value)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->Update(value, bind.buf.offset, sizeof(Math::matrix44));
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(Math::matrix44), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetMatrixArray(VariableBinding& bind, const Math::matrix44* values, SizeT count)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->UpdateArray(values, bind.buf.offset, sizeof(Math::matrix44), count);
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(Math::matrix44) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetBool(VariableBinding& bind, bool value)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->Update(value, bind.buf.offset, sizeof(bool));
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(bool), value);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetBoolArray(VariableBinding& bind, const bool* values, SizeT count)
{
	if (bind.isbuffer)
		bind.buf.uniformBuffer->UpdateArray(values, bind.buf.offset, sizeof(bool), count);
	else
		VkShaderVariable::UpdatePushRange(bind.push, sizeof(bool) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetTexture(VariableBinding& bind, ResourceBinding& res, Util::Array<VkWriteDescriptorSet>& writes, const Resources::ResourceId tex)
{
	Ids::Id24 resId = Ids::Id::GetBig(Ids::Id::GetLow(tex));
	VkTexture::RuntimeInfo& info = VkTexture::textureAllocator.GetSafe<0>(resId);

	// only change if there is a difference
	if (info.view != res.write.img.imageView)
	{
		res.write.img.imageView = info.view;

		// if image can be set as an integer, do it
		if (res.textureIsHandle)
		{
			// update texture id
			if (bind.isbuffer)
				bind.buf.uniformBuffer->Update(info.bind, bind.buf.offset, sizeof(uint32_t));
			else
				VkShaderVariable::UpdatePushRange(bind.push, sizeof(uint32_t), info.bind);
		}
		else
		{
			// dependent on type of variable, select if sampler should be coupled with sampler or if it will be assembled in-shader
			const VkImageView& view = info.view;
			VkDescriptorType type;
			if (info.type == TextureType)		type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			else if (info.type == SamplerType)	type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			else
			{
				n_error("Variable '%d' is must be either Texture or Integer to be assigned a texture!\n", res.setBinding);
			}

			VkWriteDescriptorSet set;
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.pNext = NULL;
			set.descriptorCount = 1;
			set.descriptorType = type;
			set.dstArrayElement = 0;
			set.dstBinding = res.setBinding;
			set.dstSet = res.set;
			set.pBufferInfo = NULL;
			set.pTexelBufferView = NULL;
			set.pImageInfo = &res.write.img;
			res.write.img.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			res.write.img.sampler = VK_NULL_HANDLE;

			// add to shader to update on next update
			writes.Append(set);
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetConstantBuffer(ResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ConstantBuffer>& buf)
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
		set.dstBinding = bind.setBinding;
		set.dstSet = bind.set;
		set.pBufferInfo = &this->buf;
		set.pTexelBufferView = NULL;
		set.pImageInfo = NULL;
		this->buf.buffer = buf->GetVkBuffer();
		this->buf.offset = 0;
		this->buf.range = VK_WHOLE_SIZE;

		// add to shader to update on next update
		writes.Append(set);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetShaderReadWriteTexture(ResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex)
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
		set.dstBinding = bind.setBinding;
		set.dstSet = bind.set;
		set.pBufferInfo = NULL;
		set.pTexelBufferView = NULL;
		set.pImageInfo = &this->img;
		this->img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		this->img.imageView = tex->GetVkImageView();
		this->img.sampler = VK_NULL_HANDLE;

		// add to shader to update on next update
		writes.Append(set);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetShaderReadWriteTexture(ResourceBinding& bind, Util::Array<VkWriteDescriptorSet>& writes, const Ptr<CoreGraphics::Texture>& tex)
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
		set.dstBinding = bind.setBinding;
		set.dstSet = bind.set;
		set.pBufferInfo = NULL;
		set.pTexelBufferView = NULL;
		set.pImageInfo = &this->img;
		this->img.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		this->img.imageView = tex->GetVkImageView();
		this->img.sampler = VK_NULL_HANDLE;

		// add to shader to update on next update
		writes.Append(set);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariable::SetShaderReadWriteBuffer(ResourceBinding& bind, const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf)
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
		set.dstBinding = bind.setBinding;
		set.dstSet = bind.set;
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
