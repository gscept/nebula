//------------------------------------------------------------------------------
//  surfaceconstant.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "surfaceconstant.h"
#include "coregraphics/shaderstate.h"
#include "resources/resourcemanager.h"

using namespace CoreGraphics;
using namespace Util;
namespace Materials
{
__ImplementClass(Materials::SurfaceConstant, 'SUCO', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
SurfaceConstant::SurfaceConstant() :
    system(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
SurfaceConstant::~SurfaceConstant()
{
	// empty
}

//------------------------------------------------------------------------------
/**
	Sets up a surface constant.

	Since surface constants implements a series of shader variable instances, this function will set up the constant to be 
	applicable in every shader used by the surface.

	The value is set within the shader variable instances directly, however if the shader variable instance use a constant buffer,
	that constant buffer will be updated directly.

	If the value is a texture or buffer, the shader variable instance will apply it whenever we run the SurfaceConstant::Apply.
*/
void
SurfaceConstant::Setup(const StringAtom& name, const Util::Array<Material::MaterialPass>& passes, const Util::Array<Ptr<CoreGraphics::ShaderState>>& shaders)
{
    this->name = name;

	IndexT passIndex;
	for (passIndex = 0; passIndex < passes.Size(); passIndex++)
    {
        // get shader and variable (the variable must exist in the shader!)
		const Ptr<CoreGraphics::ShaderState>& shader = shaders[passes[passIndex].index];
		bool activeVar = shader->HasVariableByName(name);

		// setup variable
		Ptr<CoreGraphics::ShaderVariable> var = 0;

		// if the variable is active, create an instance and bind its value, otherwise use a null pointer
		if (activeVar)
		{
			var = shader->GetVariableByName(this->name);
			if (var->IsActive()) this->ApplyToShaderVariable(this->value, var);
#ifndef PUBLIC_BUILD
			/*
			if (var->GetType() != this->value.GetType()) n_warning("[WARNING]: Surface constant '%s' is type '%s' but is provided with a '%s'. Behaviour is undefined (crash/corruption).\n",
				this->name.Value(),
				ShaderVariable::TypeToString(var->GetType()).AsCharPtr(),
				Variant::TypeToString(this->value.GetType()).AsCharPtr());
				*/
#endif
		}

		// setup constant, first we check if the shader even has this variable, it will be inactive otherwise
		this->bindingsByIndex.Append(ConstantBinding{ activeVar, var, shader });
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstant::Discard()
{
	this->bindingsByIndex.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstant::Apply(const IndexT passIndex)
{
    /*
	if (this->bindingsByIndex[passIndex].active)
	{
		this->ApplyToShaderVariable(this->value, this->bindingsByIndex[passIndex].var);
	}
     */
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstant::SetValue(const Util::Variant& value)
{
	if (this->value != value)
	{
		this->value = value;
		IndexT i;
		for (i = 0; i < this->bindingsByIndex.Size(); i++)
		{
			const ConstantBinding& binding = this->bindingsByIndex[i];
			if (binding.active)
			{
				if (binding.var->IsActive()) this->ApplyToShaderVariable(value, binding.var);

#ifndef PUBLIC_BUILD
				if (binding.var->GetType() != this->value.GetType()) n_warning("[WARNING]: Surface constant '%s' is type '%s' but is provided with a '%s'. Behaviour is undefined (crash/corruption).\n",
					this->name.Value(),
					ShaderVariable::TypeToString(binding.var->GetType()),
					Variant::TypeToString(this->value.GetType()));
#endif
			}
		}

	}
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstant::SetTexture(const Ptr<CoreGraphics::Texture>& tex)
{
	if (this->value.GetObject() != tex)
	{
		this->value.SetObject(tex);
		IndexT i;
		for (i = 0; i < this->bindingsByIndex.Size(); i++)
		{
			const ConstantBinding& binding = this->bindingsByIndex[i];
			if (binding.active) binding.var->SetTexture(tex);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
SurfaceConstant::ApplyToShaderVariable(const Util::Variant& value, const Ptr<CoreGraphics::ShaderVariable>& var)
{
	switch (value.GetType())
	{
	case Util::Variant::Int:
		var->SetInt(value.GetInt());
		break;
	case Util::Variant::UInt:
		var->SetInt(value.GetUInt());
		break;
	case Util::Variant::Float:
		var->SetFloat(value.GetFloat());
		break;
	case Util::Variant::Bool:
		var->SetBool(value.GetBool());
		break;
	case Util::Variant::Float2:
		var->SetFloat2(value.GetFloat2());
		break;
	case Util::Variant::Float4:
		var->SetFloat4(value.GetFloat4());
		break;
	case Util::Variant::Matrix44:
		var->SetMatrix(value.GetMatrix44());
		break;
	case Util::Variant::IntArray:
	{
		const Util::Array<int>& arr = value.GetIntArray();
		var->SetIntArray(&arr[0], arr.Size());
		break;
	}
	case Util::Variant::FloatArray:
	{
		const Util::Array<float>& arr = value.GetFloatArray();
		var->SetFloatArray(&arr[0], arr.Size());
		break;
	}
	case Util::Variant::BoolArray:
	{
		const Util::Array<bool>& arr = value.GetBoolArray();
		var->SetBoolArray(&arr[0], arr.Size());
		break;
	}
	case Util::Variant::Float2Array:
	{
		const Util::Array<Math::float2>& arr = value.GetFloat2Array();
		var->SetFloat2Array(&arr[0], arr.Size());
		break;
	}
	case Util::Variant::Float4Array:
	{
		const Util::Array<Math::float4>& arr = value.GetFloat4Array();
		var->SetFloat4Array(&arr[0], arr.Size());
		break;
	}
	case Util::Variant::Matrix44Array:
	{
		const Util::Array<Math::matrix44>& arr = value.GetMatrix44Array();
		var->SetMatrixArray(&arr[0], arr.Size());
		break;
	}
	case Util::Variant::Object:
	{
		var->SetTexture((CoreGraphics::Texture*)this->value.GetObject());
		break;
	}
	}
}

} // namespace Materials
