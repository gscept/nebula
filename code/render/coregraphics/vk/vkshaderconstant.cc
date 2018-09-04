//------------------------------------------------------------------------------
// vkshaderconstant.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderconstant.h"
#include "vkshaderstate.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/texture.h"
#include "coregraphics/rendertexture.h"
#include "vktexture.h"
#include "vkconstantbuffer.h"
#include "vkshaderrwtexture.h"
#include "vkshaderrwbuffer.h"
#include "vkshaderstate.h"
#include "vkrendertexture.h"


using namespace CoreGraphics;
namespace Vulkan
{

//------------------------------------------------------------------------------
/**
*/
void
VkShaderConstantBindToUniformBuffer(const CoreGraphics::ShaderConstantId var, CoreGraphics::ConstantBufferId buffer, VkShaderConstantAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue)
{
	VkShaderConstantMemoryBinding& binding = allocator.Get<0>(var.id);
	binding.backing.uniformBuffer = buffer.HashCode();
	binding.offset = offset;
	binding.size = size;
	binding.defaultValue = defaultValue;
	binding.isvalid = true;
	binding.isbuffer = true;

	// make sure that the buffer is updated (data is array since we have a char*)
	ConstantBufferUpdate(buffer, defaultValue, offset, size);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderConstantBindToPushConstantRange(const CoreGraphics::ShaderConstantId var, uint8_t* buffer, VkShaderConstantAllocator& allocator, uint32_t offset, uint32_t size, int8_t* defaultValue)
{
	VkShaderConstantMemoryBinding& binding = allocator.Get<0>(var.id);
	binding.backing.push = buffer;
	binding.offset = offset;
	binding.size = size;
	binding.defaultValue = defaultValue;
	binding.isvalid = true;
	binding.isbuffer = false;
	
	// copy data to buffer
	VkShaderVariableUpdatePushRange(buffer + offset, size, defaultValue);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderVariableGetBinding(const CoreGraphics::ShaderConstantId var, VkShaderConstantAllocator& allocator)
{
	const VkShaderConstantDescriptorBinding& binding = allocator.Get<1>(var.id);
	return binding.setBinding;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderConstantSetup(AnyFX::VkVariable* var, Ids::Id24 id, VkShaderConstantAllocator& allocator, const CoreGraphics::ResourceTableId& tid)
{
	n_assert(0 != var);

	Util::String name = var->name.c_str();

	VkShaderConstantMemoryBinding& varBind = allocator.Get<0>(id);
	VkShaderConstantDescriptorBinding& resBind = allocator.Get<1>(id);
	VkShaderConstantSetupInfo& setupInfo = allocator.Get<2>(id);
	varBind.backing.push = nullptr;
	varBind.isbuffer = false;
	varBind.size = 0;
	varBind.offset = 0;
	varBind.defaultValue = nullptr;
	resBind.textureIsHandle = false;
	resBind.dynamicOffset = 0;
	resBind.setBinding = -1;
	
	setupInfo.name = name;
	resBind.set = tid;
	switch (var->type)
	{
	case AnyFX::Double:
	case AnyFX::Float:
		setupInfo.type = FloatVariableType;
		break;
	case AnyFX::Short:
	case AnyFX::Integer:
	case AnyFX::UInteger:
		setupInfo.type = IntVariableType;
		break;
	case AnyFX::Bool:
		setupInfo.type = BoolVariableType;
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
		setupInfo.type = VectorVariableType;
		break;
	case AnyFX::Float2:
	case AnyFX::Double2:
	case AnyFX::Integer2:
	case AnyFX::UInteger2:
	case AnyFX::Short2:
	case AnyFX::Bool2:
		setupInfo.type = Vector2VariableType;
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
		setupInfo.type = MatrixVariableType;
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
		setupInfo.type = ImageReadWriteVariableType;
		n_assert(tid != ResourceTableId::Invalid());
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
		setupInfo.type = SamplerVariableType;
		n_assert(tid != ResourceTableId::Invalid());
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
		setupInfo.type = TextureVariableType;
		n_assert(tid != ResourceTableId::Invalid());
		resBind.setBinding = var->bindingLayout.binding;
		break;
	case AnyFX::TextureHandle:
		setupInfo.type = TextureVariableType;
		resBind.textureIsHandle = true;
		break;
	case AnyFX::ImageHandle:
		setupInfo.type = ImageReadWriteVariableType;
		resBind.textureIsHandle = true;
		break;
	case AnyFX::SamplerHandle:
		setupInfo.type = SamplerVariableType;
		resBind.textureIsHandle = true;
		break;
	default:
		setupInfo.type = ConstantBufferVariableType;
		n_assert(tid != ResourceTableId::Invalid());
		resBind.setBinding = var->bindingLayout.binding;
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderConstantSetup(AnyFX::VkVarbuffer* var, Ids::Id24 id, VkShaderConstantAllocator& allocator, const CoreGraphics::ResourceTableId& tid)
{
	n_assert(0 != var);
	Util::String name = var->name.c_str();
	VkShaderConstantMemoryBinding& varBind = allocator.Get<0>(id);
	VkShaderConstantDescriptorBinding& resBind = allocator.Get<1>(id);
	VkShaderConstantSetupInfo& setupInfo = allocator.Get<2>(id);

	setupInfo.name = name;
	setupInfo.type = BufferReadWriteVariableType;
	resBind.dynamicOffset = var->set == NEBULAT_DYNAMIC_OFFSET_GROUP;
	resBind.setBinding = var->bindingLayout.binding;
	resBind.set = tid;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderConstantSetup(AnyFX::VkVarblock* var, Ids::Id24 id, VkShaderConstantAllocator& allocator, const CoreGraphics::ResourceTableId& tid)
{
	n_assert(0 != var);
	Util::String name = var->name.c_str();
	VkShaderConstantMemoryBinding& varBind = allocator.Get<0>(id);
	VkShaderConstantDescriptorBinding& resBind = allocator.Get<1>(id);
	VkShaderConstantSetupInfo& setupInfo = allocator.Get<2>(id);
	
	setupInfo.name = name;
	setupInfo.type = ConstantBufferVariableType;
	resBind.dynamicOffset = var->set == NEBULAT_DYNAMIC_OFFSET_GROUP;
	resBind.setBinding = var->bindingLayout.binding;
	resBind.set = tid;
}

//------------------------------------------------------------------------------
/**
*/
void
SetInt(VkShaderConstantMemoryBinding& bind, int value)
{
	if (bind.isbuffer)
		ConstantBufferUpdate(bind.backing.uniformBuffer, &value, bind.offset, sizeof(int));
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(int), value);
}

//------------------------------------------------------------------------------
/**
*/
void
SetIntArray(VkShaderConstantMemoryBinding& bind, const int* values, SizeT count)
{
	if (bind.isbuffer)
		ConstantBufferArrayUpdate(bind.backing.uniformBuffer, values, bind.offset, sizeof(int), count);
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(int) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
SetUInt(VkShaderConstantMemoryBinding& bind, uint value)
{
	if (bind.isbuffer)
		ConstantBufferUpdate(bind.backing.uniformBuffer, &value, bind.offset, sizeof(uint));
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(uint), value);
}

//------------------------------------------------------------------------------
/**
*/
void
SetUIntArray(VkShaderConstantMemoryBinding& bind, const uint* values, SizeT count)
{
	if (bind.isbuffer)
		ConstantBufferArrayUpdate(bind.backing.uniformBuffer, values, bind.offset, sizeof(uint), count);
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(uint) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
SetFloat(VkShaderConstantMemoryBinding& bind, float value)
{
	if (bind.isbuffer)
		ConstantBufferUpdate(bind.backing.uniformBuffer, &value, bind.offset, sizeof(float));
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(float), value);
}

//------------------------------------------------------------------------------
/**
*/
void
SetFloatArray(VkShaderConstantMemoryBinding& bind, const float* values, SizeT count)
{
	if (bind.isbuffer)
		ConstantBufferArrayUpdate(bind.backing.uniformBuffer, values, bind.offset, sizeof(float), count);
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(float) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
SetFloat2(VkShaderConstantMemoryBinding& bind, const Math::float2& value)
{
	if (bind.isbuffer)
		ConstantBufferUpdate(bind.backing.uniformBuffer, &value, bind.offset, sizeof(Math::float2));
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(Math::float2), value);
}

//------------------------------------------------------------------------------
/**
*/
void
SetFloat2Array(VkShaderConstantMemoryBinding& bind, const Math::float2* values, SizeT count)
{
	if (bind.isbuffer)
		ConstantBufferArrayUpdate(bind.backing.uniformBuffer, values, bind.offset, sizeof(Math::float2), count);
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(Math::float2) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
SetFloat4(VkShaderConstantMemoryBinding& bind, const Math::float4& value)
{
	if (bind.isbuffer)
		ConstantBufferUpdate(bind.backing.uniformBuffer, &value, bind.offset, sizeof(Math::float4));
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(Math::float4), value);
}

//------------------------------------------------------------------------------
/**
*/
void
SetFloat4Array(VkShaderConstantMemoryBinding& bind, const Math::float4* values, SizeT count)
{
	if (bind.isbuffer)
		ConstantBufferArrayUpdate(bind.backing.uniformBuffer, values, bind.offset, sizeof(Math::float4), count);
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(Math::float4) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
SetMatrix(VkShaderConstantMemoryBinding& bind, const Math::matrix44& value)
{
	if (bind.isbuffer)
		ConstantBufferUpdate(bind.backing.uniformBuffer, &value, bind.offset, sizeof(Math::matrix44));
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(Math::matrix44), value);
}

//------------------------------------------------------------------------------
/**
*/
void
SetMatrixArray(VkShaderConstantMemoryBinding& bind, const Math::matrix44* values, SizeT count)
{
	if (bind.isbuffer)
		ConstantBufferArrayUpdate(bind.backing.uniformBuffer, values, bind.offset, sizeof(Math::matrix44), count);
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(Math::matrix44) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
SetBool(VkShaderConstantMemoryBinding& bind, bool value)
{
	if (bind.isbuffer)
		ConstantBufferUpdate(bind.backing.uniformBuffer, &value, bind.offset, sizeof(bool));
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(bool), value);
}

//------------------------------------------------------------------------------
/**
*/
void
SetBoolArray(VkShaderConstantMemoryBinding& bind, const bool* values, SizeT count)
{
	if (bind.isbuffer)
		ConstantBufferArrayUpdate(bind.backing.uniformBuffer, values, bind.offset, sizeof(bool), count);
	else
		VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(bool) * count, values);
}

//------------------------------------------------------------------------------
/**
*/
void
SetTexture(VkShaderConstantMemoryBinding& bind, VkShaderConstantDescriptorBinding& res, const CoreGraphics::TextureId tex)
{
	VkTextureRuntimeInfo& info = textureAllocator.GetSafe<0>(tex.allocId);

	// only change if there is a difference
	if (info.view != res.write.img.imageView)
	{
		res.write.img.imageView = info.view;

		// if image can be set as an integer, do it
		if (res.textureIsHandle)
		{
			// update texture id
			if (bind.isbuffer)
				ConstantBufferUpdate(bind.backing.uniformBuffer, &info.bind, bind.offset, sizeof(uint32_t));
			else
				VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(uint32_t), info.bind);
		}
		else
		{
			CoreGraphics::ResourceTableTexture info;
			info.isDepth = false;
			info.sampler = CoreGraphics::SamplerId::Invalid();
			info.slot = res.setBinding;
			info.tex = tex;
			info.index = 0;

			CoreGraphics::ResourceTableSetTexture(res.set, info);
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
SetRenderTexture(VkShaderConstantMemoryBinding& bind, VkShaderConstantDescriptorBinding& res, const CoreGraphics::RenderTextureId tex)
{
	VkRenderTextureRuntimeInfo& info = renderTextureAllocator.Get<1>(tex.id24);

	// only change if there is a difference
	if (info.view != res.write.img.imageView)
	{
		res.write.img.imageView = info.view;

		// if image can be set as an integer, do it
		if (res.textureIsHandle)
		{
			// update texture id
			if (bind.isbuffer)
				ConstantBufferUpdate(bind.backing.uniformBuffer, &info.bind, bind.offset, sizeof(uint32_t));
			else
				VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(uint32_t), info.bind);
		}
		else
		{
			CoreGraphics::ResourceTableRenderTexture info;
			info.isDepth = false;
			info.sampler = CoreGraphics::SamplerId::Invalid();
			info.slot = res.setBinding;
			info.tex = tex;
			info.index = 0;

			CoreGraphics::ResourceTableSetTexture(res.set, info);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SetConstantBuffer(VkShaderConstantDescriptorBinding& bind, const CoreGraphics::ConstantBufferId buf)
{
	VkConstantBufferRuntimeInfo& info = constantBufferAllocator.Get<0>(buf.id24);
	if (info.buf != bind.write.buf.buffer)
	{
		CoreGraphics::ResourceTableConstantBuffer info;
		info.buf = buf;
		info.offset = 0;
		info.size = -1;
		info.dynamicOffset = bind.dynamicOffset;
		info.slot = bind.setBinding;
		info.index = 0;
		info.texelBuffer = false;

		CoreGraphics::ResourceTableSetConstantBuffer(bind.set, info);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SetShaderReadWriteTexture(VkShaderConstantMemoryBinding& bind, VkShaderConstantDescriptorBinding& res, const CoreGraphics::ShaderRWTextureId tex)
{
	VkShaderRWTextureRuntimeInfo& info = shaderRWTextureAllocator.Get<1>(tex.id24);
	if (info.view != res.write.img.imageView)
	{
		// if image can be set as an integer, do it
		if (res.textureIsHandle)
		{
			// update texture id
			if (bind.isbuffer)
				ConstantBufferUpdate(bind.backing.uniformBuffer, &info.bind, bind.offset, sizeof(uint32_t));
			else
				VkShaderVariableUpdatePushRange(bind.backing.push, sizeof(uint32_t), info.bind);
		}
		else
		{
			CoreGraphics::ResourceTableShaderRWTexture info;
			info.sampler = SamplerId::Invalid();
			info.slot = res.setBinding;
			info.tex = tex;
			info.index = 0;

			CoreGraphics::ResourceTableSetShaderRWTexture(res.set, info);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SetShaderReadWriteTexture(VkShaderConstantDescriptorBinding& bind, const CoreGraphics::TextureId tex)
{
	VkTextureRuntimeInfo& info = textureAllocator.GetSafe<0>(tex.allocId);
	if (info.view != bind.write.img.imageView)
	{
		CoreGraphics::ResourceTableTexture info;
		info.sampler = SamplerId::Invalid();
		info.slot = bind.setBinding;
		info.tex = tex;
		info.isDepth = false;
		info.index = 0;

		CoreGraphics::ResourceTableSetShaderRWTexture(bind.set, info);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SetShaderReadWriteBuffer(VkShaderConstantDescriptorBinding& bind, const CoreGraphics::ShaderRWBufferId buf)
{
	VkShaderRWBufferRuntimeInfo& info = shaderRWBufferAllocator.Get<1>(buf.id24);
	if (info.buf != bind.write.buf.buffer)
	{
		CoreGraphics::ResourceTableShaderRWBuffer info;
		info.buf = buf;
		info.offset = 0;
		info.size = -1;
		info.dynamicOffset = bind.dynamicOffset;
		info.texelBuffer = false;
		info.slot = bind.setBinding;
		info.index = 0;

		CoreGraphics::ResourceTableSetShaderRWBuffer(bind.set, info);
	}
}

} // namespace Vulkan
