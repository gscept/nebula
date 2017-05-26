//------------------------------------------------------------------------------
// vkshadervariableinstance.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkshadervariableinstance.h"

using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderVariableInstance, 'VKVI', Base::ShaderVariableInstanceBase);
//------------------------------------------------------------------------------
/**
*/
VkShaderVariableInstance::VkShaderVariableInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkShaderVariableInstance::~VkShaderVariableInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariableInstance::Apply()
{
	n_assert(this->shaderVariable.isvalid());
	switch (this->value.GetType())
	{
	case Util::Variant::Object:
	{
		Core::RefCounted* obj = this->value.GetObject();
		if (obj != 0)
		{
			switch (this->type)
			{
			case TextureObjectType:
				this->shaderVariable->SetTexture((CoreGraphics::Texture*)this->value.GetObject());
				break;
			case ConstantBufferObjectType:
				this->shaderVariable->SetConstantBuffer((CoreGraphics::ConstantBuffer*)this->value.GetObject());
				break;
			case ReadWriteImageObjectType:
				if (obj->IsA(ShaderReadWriteTexture::RTTI))
				{
					this->shaderVariable->SetShaderReadWriteTexture((ShaderReadWriteTexture*)obj);
				}
				else
				{
					this->shaderVariable->SetShaderReadWriteTexture((Texture*)obj);
				}
				break;
			case ReadWriteBufferObjectType:
				this->shaderVariable->SetShaderReadWriteBuffer((CoreGraphics::ShaderReadWriteBuffer*)this->value.GetObject());
				break;
			}
			break;
		}
	}
	default:
		ShaderVariableInstanceBase::Apply();
		break;
	}
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderVariableInstance::ApplyTo(const Ptr<CoreGraphics::ShaderVariable>& var)
{
	n_assert(var.isvalid());
	switch (this->value.GetType())
	{
	case Util::Variant::Object:
	{
		Core::RefCounted* obj = this->value.GetObject();
		if (obj != 0)
		{
			switch (this->type)
			{
			case TextureObjectType:
				var->SetTexture((CoreGraphics::Texture*)this->value.GetObject());
				break;
			case ConstantBufferObjectType:
				var->SetConstantBuffer((CoreGraphics::ConstantBuffer*)this->value.GetObject());
				break;
			case ReadWriteImageObjectType:
				if (obj->IsA(ShaderReadWriteTexture::RTTI))
				{
					var->SetShaderReadWriteTexture((ShaderReadWriteTexture*)obj);
				}
				else
				{
					var->SetShaderReadWriteTexture((Texture*)obj);
				}
				break;
			case ReadWriteBufferObjectType:
				var->SetShaderReadWriteBuffer((CoreGraphics::ShaderReadWriteBuffer*)this->value.GetObject());
				break;
			}
		}
	}
	default:
		ShaderVariableInstanceBase::Apply();
		break;
	}
}

} // namespace Vulkan