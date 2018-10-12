//------------------------------------------------------------------------------
//  shadervariablebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shadervariableinstance.h"
#include "coregraphics/shader.h"

namespace Base
{
__ImplementClass(Base::ShaderVariableBase, 'SVRB', Core::RefCounted);

using namespace CoreGraphics;
using namespace Math;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ShaderVariableBase::ShaderVariableBase() :
    type(UnknownType)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ShaderVariableBase::~ShaderVariableBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ShaderVariableInstance>
ShaderVariableBase::CreateInstance() 
{
    Ptr<ShaderVariableInstance> newInst = ShaderVariableInstance::Create();
    Ptr<ShaderVariableBase> thisPtr(this);
    newInst->Setup(thisPtr.downcast<ShaderVariable>());
	this->instances.Append(newInst);
    return newInst;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::DiscardInstance(const Ptr<CoreGraphics::ShaderVariableInstance>& inst)
{
	inst->Cleanup();
	IndexT i = this->instances.FindIndex(inst);
	n_assert(i != InvalidIndex);
	this->instances.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
String
ShaderVariableBase::TypeToString(Type t)
{
    switch (t)
    {
        case UnknownType:			return "unknown";
        case IntType:				return "int";
        case FloatType:				return "float";
        case VectorType:			return "vector";
		case Vector2Type:			return "vector2";
        case MatrixType:			return "matrix";
        case BoolType:				return "bool";
        case TextureType:			return "texture";
		case SamplerType:			return "sampler";
		case ConstantBufferType:	return "constantbuffer";
		case ImageReadWriteType:	return "image";
		case BufferReadWriteType:	return "storagebuffer";
        default:
            n_error("ShaderVariableBase::TypeToString(): invalid type code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
ShaderVariableBase::Type
ShaderVariableBase::StringToType(const String& str)
{
    if (str == "int") return IntType;
    else if (str == "float") return FloatType;
    else if (str == "vector") return VectorType;
	else if (str == "vector2") return Vector2Type;
    else if (str == "matrix") return MatrixType;
    else if (str == "bool") return BoolType;
    else if (str == "texture") return TextureType;
	else if (str == "sampler") return SamplerType;
	else if (str == "constantbuffer") return ConstantBufferType;
	else if (str == "image") return ImageReadWriteType;
	else if (str == "storagebuffer") return BufferReadWriteType;
    else
    {
        n_error("ShaderVariable::StringToType(): invalid string '%s'!\n", str.AsCharPtr());
        return UnknownType;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetInt(int value)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetIntArray(const int* values, SizeT count)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetFloat(float value)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetFloatArray(const float* values, SizeT count)
{
    // empty, override in subclass
}


//------------------------------------------------------------------------------
/**
*/
void 
ShaderVariableBase::SetFloat2(const float2& value)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderVariableBase::SetFloat2Array(const float2* value, SizeT count)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetFloat4(const float4& value)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetFloat4Array(const float4* values, SizeT count)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetMatrix(const matrix44& value)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetMatrixArray(const matrix44* values, SizeT count)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetBool(bool value)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetBoolArray(const bool* values, SizeT count)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetTexture(const Ptr<Texture>& value)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetConstantBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buf)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetShaderReadWriteTexture(const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetShaderReadWriteTexture(const Ptr<CoreGraphics::Texture>& tex)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderVariableBase::SetShaderReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderVariableBase::Cleanup()
{
	IndexT i;
	for (i = 0; i < this->instances.Size(); i++)
	{
		this->instances[i]->Cleanup();
	}
	this->instances.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderVariableBase::Reload()
{
	// empty, override in le subclass
}

//------------------------------------------------------------------------------
/**
*/
const bool
ShaderVariableBase::IsActive() const
{
	return false;
}

} // namespace Base
