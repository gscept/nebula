//------------------------------------------------------------------------------
//  shadervariableinstancebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/shadervariableinstancebase.h"
#include "coregraphics/shadervariableinstance.h"
#include "coregraphics/shadervariable.h"
#include "resources/resourcemanager.h"

namespace Base
{
__ImplementClass(Base::ShaderVariableInstanceBase, 'SVIB', Core::RefCounted);

using namespace CoreGraphics;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
ShaderVariableInstanceBase::ShaderVariableInstanceBase() :
	shaderVariable(0),
	type(NoObjectType)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderVariableInstanceBase::~ShaderVariableInstanceBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableInstanceBase::Setup(const Ptr<ShaderVariable>& var)
{
	this->Prepare(var->GetType());
	this->shaderVariable = var;    
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableInstanceBase::Setup(const Base::ShaderVariableBase::Type& type)
{
	this->Prepare(type);
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderVariableInstanceBase::Discard()
{
	this->shaderVariable->DiscardInstance((ShaderVariableInstance*)this);
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderVariableInstanceBase::Cleanup()
{
	n_assert(this->shaderVariable.isvalid());
	this->shaderVariable = nullptr;

	// clear value
	this->value.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableInstanceBase::Prepare(ShaderVariable::Type type)
{
    n_assert(!this->shaderVariable.isvalid());
    switch (type)
    {
        case ShaderVariable::IntType:
            this->value.SetType(Variant::Int);
            break;
        
        case ShaderVariable::FloatType:
            this->value.SetType(Variant::Float);
            break;

        case ShaderVariable::VectorType:
            this->value.SetType(Variant::Float4);
            break;

		case ShaderVariable::Vector2Type:
			this->value.SetType(Variant::Float2);
			break;

        case ShaderVariable::MatrixType:
            this->value.SetType(Variant::Matrix44);
            break;

        case ShaderVariable::BoolType:
            this->value.SetType(Variant::Bool);
            break;

        case ShaderVariable::TextureType:
			this->type = TextureObjectType;
			this->value.SetType(Variant::Object);
			break;
		case ShaderVariable::ImageReadWriteType:
			this->type = ReadWriteImageObjectType;
			this->value.SetType(Variant::Object);
			break;
		case ShaderVariable::ConstantBufferType:
			this->type = ConstantBufferObjectType;
            this->value.SetType(Variant::Object);
            break;

        default:
            n_error("ShaderVariableInstanceBase::Prepare(): Invalid ShaderVariable::Type in switch!\n");
            break;
    }
}

//------------------------------------------------------------------------------
/**
    Switch-case looks complicated, however sub classing shader variable for every type is too much overhead/bloat to be worth it
*/
void
ShaderVariableInstanceBase::Apply()
{
    n_assert(this->shaderVariable.isvalid());
    switch (this->value.GetType())
    {
        case Variant::Int:
            this->shaderVariable->SetInt(this->value.GetInt());
            break;
        case Variant::IntArray:
        {
            Array<int> array = this->value.GetIntArray();
            this->shaderVariable->SetIntArray((const int*)&array, array.Size());
            break;
        }
        case Variant::Float:
            this->shaderVariable->SetFloat(this->value.GetFloat());
            break;
        case Variant::FloatArray:
        {
            Array<float> array = this->value.GetFloatArray();
            this->shaderVariable->SetFloatArray((const float*)&array, array.Size());
            break;
        }
        case Variant::Float2:
            this->shaderVariable->SetFloat2(this->value.GetFloat2());
            break;
        case Variant::Float2Array:
        {
            Array<Math::float2> array = this->value.GetFloat2Array();
            this->shaderVariable->SetFloat2Array((const Math::float2*)&array, array.Size());
            break;
        }
        case Variant::Float4:
            this->shaderVariable->SetFloat4(this->value.GetFloat4());
            break;
        case Variant::Float4Array:
        {
            Array<Math::float4> array = this->value.GetFloat4Array();
            this->shaderVariable->SetFloat4Array((const Math::float4*)&array, array.Size());
            break;
        }
        case Variant::Matrix44:
            this->shaderVariable->SetMatrix(this->value.GetMatrix44());
            break;
        case Variant::Matrix44Array:
        {
            Array<Math::matrix44> array = this->value.GetMatrix44Array();
            this->shaderVariable->SetMatrixArray((const Math::matrix44*)&array, array.Size());
            break;
        }
        case Variant::Bool:
            this->shaderVariable->SetBool(this->value.GetBool());
            break;
        case Variant::BoolArray:
        {
            Array<bool> array = this->value.GetBoolArray();
            this->shaderVariable->SetBoolArray((const bool*)&array, array.Size());
            break;
        }
        case Variant::Object:
            // @note: implicit Ptr<> creation!
            if (this->value.GetObject() != 0)
            {
				switch (this->type)
				{
				case TextureObjectType:
					this->shaderVariable->SetTexture((Texture*)this->value.GetObject());
					break;
				case ConstantBufferObjectType:
					this->shaderVariable->SetConstantBuffer((ConstantBuffer*)this->value.GetObject());
					break;
				case ReadWriteImageObjectType:
				{
					Core::RefCounted* obj = this->value.GetObject();
					if (obj->IsA(ShaderReadWriteTexture::RTTI))
					{
						this->shaderVariable->SetShaderReadWriteTexture((ShaderReadWriteTexture*)obj);
					}
					else
					{
						this->shaderVariable->SetShaderReadWriteTexture((Texture*)obj);
					}
					break;
				}
				case ReadWriteBufferObjectType:
					this->shaderVariable->SetShaderReadWriteBuffer((ShaderReadWriteBuffer*)this->value.GetObject());
					break;
				default:
					n_error("ShaderVariableInstanceBase::Apply(): Improper object type");
					break;
				}
            }
            break;
        default:
            n_error("ShaderVariableInstance::Apply(): invalid data type!");
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderVariableInstanceBase::ApplyTo(const Ptr<CoreGraphics::ShaderVariable>& var)
{
	n_assert(var.isvalid());
	switch (this->value.GetType())
	{
	case Variant::Int:
		var->SetInt(this->value.GetInt());
		break;
    case Variant::IntArray:
    {
        const Array<int>& array = this->value.GetIntArray();
        var->SetIntArray((const int*)&array, array.Size());
        break;
    }
	case Variant::Float:
		var->SetFloat(this->value.GetFloat());
		break;
    case Variant::FloatArray:
    {
        const Array<float>& array = this->value.GetFloatArray();
        var->SetFloatArray((const float*)&array, array.Size());
        break;
    }
    case Variant::Float2:
        var->SetFloat2(this->value.GetFloat2());
        break;
    case Variant::Float2Array:
    {
        const Array<Math::float2>& array = this->value.GetFloat2Array();
        var->SetFloat2Array((const Math::float2*)&array, array.Size());
        break;
    }
	case Variant::Float4:
		var->SetFloat4(this->value.GetFloat4());
		break;
    case Variant::Float4Array:
    {
        const Array<Math::float4>& array = this->value.GetFloat4Array();
        var->SetFloat4Array((const Math::float4*)&array, array.Size());
        break;
    }
	case Variant::Matrix44:
		var->SetMatrix(this->value.GetMatrix44());
		break;
    case Variant::Matrix44Array:
    {
        const Array<Math::matrix44>& array = this->value.GetMatrix44Array();
        var->SetMatrixArray((const Math::matrix44*)&array, array.Size());
        break;
    }
	case Variant::Bool:
		var->SetBool(this->value.GetBool());
		break;
    case Variant::BoolArray:
    {
        const Array<bool>& array = this->value.GetBoolArray();
        var->SetBoolArray((const bool*)&array, array.Size());
        break;
    }
	case Variant::Object:

		// @note: implicit Ptr<> creation!
        if (this->value.GetObject() != 0)
        {
			switch (this->type)
			{
			case TextureObjectType:
				var->SetTexture((Texture*)this->value.GetObject());
				break;
			case ConstantBufferObjectType:
				var->SetConstantBuffer((ConstantBuffer*)this->value.GetObject());
				break;
			case ReadWriteImageObjectType:
			{
				Core::RefCounted* obj = this->value.GetObject();
				if (obj->IsA(ShaderReadWriteTexture::RTTI))
				{
					var->SetShaderReadWriteTexture((ShaderReadWriteTexture*)obj);
				}
				else
				{
					var->SetShaderReadWriteTexture((Texture*)obj);
				}
				break;
			}
			case ReadWriteBufferObjectType:
				var->SetShaderReadWriteBuffer((ShaderReadWriteBuffer*)this->value.GetObject());
				break;
			default:
				n_error("ShaderVariableInstanceBase::Apply(): Improper object type");
				break;
			}
        }
        else
        {
			switch (this->type)
			{
			case TextureObjectType:
				var->SetTexture(NULL);
				break;
			case ConstantBufferObjectType:
				var->SetConstantBuffer(NULL);
				break;
			case ReadWriteImageObjectType:
				var->SetShaderReadWriteTexture((ShaderReadWriteTexture*)NULL);
				break;
			case ReadWriteBufferObjectType:
				var->SetShaderReadWriteBuffer(NULL);
				break;
			default:
				n_error("ShaderVariableInstanceBase::Apply(): Improper object type");
				break;
			}
        }
		break;
	default:
		n_error("ShaderVariable::Apply(): invalid data type for scalar!");
		break;
	}
}


} // namespace Base

