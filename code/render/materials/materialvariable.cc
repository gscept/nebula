//------------------------------------------------------------------------------
//  materialvariable.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materials/materialvariable.h"
#include "materials/materialvariableinstance.h"
#include "resources/managedtexture.h"

namespace Materials
{
__ImplementClass(Materials::MaterialVariable, 'MTVB', Core::RefCounted);

using namespace Math;
using namespace CoreGraphics;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
MaterialVariable::MaterialVariable()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
MaterialVariable::~MaterialVariable()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void 
MaterialVariable::Apply()
{
	IndexT i;
	for (i = 0; i < variables.Size(); i++)
	{
		switch (this->value.GetType())
		{
		case Variant::Int:
			{
				this->ApplyInt(this->value.GetInt());
				break;
			}			
		case Variant::IntArray:
			{
				Array<int> array = this->value.GetIntArray();
				this->ApplyIntArray((const int*)&array, array.Size());
				break;
			}
		case Variant::Float:
			{
				this->ApplyFloat(this->value.GetFloat());
				break;
			}			
		case Variant::FloatArray:
			{
				Array<float> array = this->value.GetFloatArray();
				this->ApplyFloatArray((const float*)&array, array.Size());
				break;
			}
		case Variant::Float2:
			{
				this->ApplyFloat2(this->value.GetFloat2());
				break;
			}			
		case Variant::Float2Array:
			{
				Array<float2> array = this->value.GetFloat2Array();
				this->ApplyFloat2Array((const float2*)&array, array.Size());
				break;
			}
		case Variant::Float4:
			{
				this->ApplyFloat4(this->value.GetFloat4());
				break;
			}
		case Variant::Float4Array:
			{
				Array<float4> array = this->value.GetFloat4Array();
				this->ApplyFloat4Array((const float4*)&array, array.Size());
				break;
			}
		case Variant::Matrix44:
			{
				this->ApplyMatrix(this->value.GetMatrix44());
				break;
			}			
		case Variant::Matrix44Array:
			{
				Array<matrix44> array = this->value.GetMatrix44Array();
				this->ApplyMatrixArray((const matrix44*)&array, array.Size());
				break;
			}
		case Variant::Bool:
			{
				this->ApplyBool(this->value.GetBool());
				break;
			}			
		case Variant::BoolArray:
			{
				Array<bool> array = this->value.GetBoolArray();
				this->ApplyBoolArray((const bool*)&array, array.Size());
				break;
			}
		case Variant::Object:
			{
				const Ptr<Core::RefCounted>& obj = this->value.GetObject();
				if (obj->IsA(Resources::ManagedResource::RTTI))
				{
					const Ptr<Resources::ManagedTexture>& tex = obj.downcast<Resources::ManagedTexture>();
					this->ApplyTexture(tex->GetTexture());
				}
				else
				{
					this->ApplyTexture(obj.downcast<CoreGraphics::Texture>());
				}
				
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
Ptr<MaterialVariableInstance> 
MaterialVariable::CreateInstance()
{
	Ptr<MaterialVariableInstance> newInst = MaterialVariableInstance::Create();
	Ptr<MaterialVariable> thisPtr(this);
	newInst->Setup(thisPtr);
	this->instances.Append(newInst);
	return newInst;
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialVariable::DiscardVariableInstance( const Ptr<MaterialVariableInstance>& inst )
{
	inst->Cleanup();
	IndexT i = this->instances.FindIndex(inst);
	n_assert(InvalidIndex != i);
	this->instances.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialVariable::Setup( const Array<Ptr<CoreGraphics::ShaderVariable> >& shaderVariables, Variant defaultValue )
{
	n_assert(this->variables.IsEmpty());
	this->variables.AppendArray(shaderVariables);

	// set the name and type from the first shader variable (they should all have the same name and type)
	this->SetName(shaderVariables[0]->GetName());
	this->SetType((Type)shaderVariables[0]->GetType());
	this->value = defaultValue;
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialVariable::Cleanup()
{
	this->variables.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyInt(int value)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetInt(value);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyIntArray(const int* values, SizeT count)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetIntArray(values, count);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyFloat(float value)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetFloat(value);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyFloatArray(const float* values, SizeT count)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetFloatArray(values, count);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyFloat2(const Math::float2& value)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetFloat2(value);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyFloat2Array(const Math::float2* values, SizeT count)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetFloat2Array(values, count);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyFloat4(const Math::float4& value)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetFloat4(value);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyFloat4Array(const Math::float4* values, SizeT count)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetFloat4Array(values, count);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyMatrix(const Math::matrix44& value)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetMatrix(value);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyMatrixArray(const Math::matrix44* values, SizeT count)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetMatrixArray(values, count);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyBool(bool value)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetBool(value);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyBoolArray(const bool* values, SizeT count)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetBoolArray(values, count);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialVariable::ApplyTexture(const Ptr<CoreGraphics::Texture>& value)
{
	for (int i = 0; i < this->variables.Size(); i++)
	{
		this->variables[i]->SetTexture(value);
	}
}

//------------------------------------------------------------------------------
/**
*/
Util::Variant::Type
MaterialVariable::VariantTypeFromType(MaterialVariable::Type type)
{
	switch (type)
	{
	case IntType:
		return Util::Variant::Int;
		break;
	case FloatType:
		return Util::Variant::Float;
		break;
	case BoolType:
		return Util::Variant::Bool;
		break;
	case VectorType:
		return Util::Variant::Float4;
		break;
	case MatrixType:
		return Util::Variant::Matrix44;
		break;
	case TextureType:
		return Util::Variant::Object;
		break;
	}
	n_error("Material Variable is of unknown type!");
	return Util::Variant::Void;
}


} // namespace Materials