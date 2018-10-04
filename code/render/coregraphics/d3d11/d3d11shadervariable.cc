//------------------------------------------------------------------------------
//  d3d11shadervariable.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/d3d11shadervariable.h"
#include "coregraphics/shadervariableinstance.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/d3d11/d3d11renderdevice.h"
#include "math/vector.h"


namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11ShaderVariable, 'D1VR', Base::ShaderVariableBase);

using namespace CoreGraphics;
using namespace Math;
//------------------------------------------------------------------------------
/**
*/
D3D11ShaderVariable::D3D11ShaderVariable() :
	reload(false)
{
    // empty
}    

//------------------------------------------------------------------------------
/**
*/
D3D11ShaderVariable::~D3D11ShaderVariable()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderVariable::Setup( ID3DX11EffectVariable* var )
{
	n_assert(0 != var);
	this->d3d11Var = var;

	D3DX11_EFFECT_VARIABLE_DESC varDesc;
	var->GetDesc(&varDesc);

	// set name and semantics
	this->SetName(Name(varDesc.Name));
	this->SetSemantic(Semantic(varDesc.Semantic));

	if (varDesc.Semantic == NULL)
	{
		this->SetSemantic(this->GetName());
	}

	// evaulate type
	ID3DX11EffectType* type = var->GetType();
	D3DX11_EFFECT_TYPE_DESC desc;
	type->GetDesc(&desc);	

	if (desc.Class == D3D10_SVC_SCALAR)
	{
		switch (desc.Type)
		{
		case D3D10_SVT_BOOL:
			this->SetType(BoolType);
			break;
		case D3D10_SVT_INT:
			this->SetType(IntType);
			break;
		case D3D10_SVT_FLOAT:
			this->SetType(FloatType);
			break;
		}
	}
	else if (desc.Class == D3D10_SVC_VECTOR)
	{
		this->SetType(VectorType);
	}
	else if (desc.Class == D3D10_SVC_MATRIX_COLUMNS)
	{
		this->SetType(MatrixType);
	}
	else if (desc.Class == D3D10_SVC_MATRIX_ROWS)
	{
		this->SetType(MatrixType);
	}
	else if (desc.Class == D3D10_SVC_OBJECT)
	{
		switch (desc.Type)
		{
		case D3D10_SVT_TEXTURE:
		case D3D10_SVT_TEXTURE1D:
		case D3D10_SVT_TEXTURE2D:
		case D3D10_SVT_TEXTURE3D:
		case D3D10_SVT_TEXTURECUBE:
			this->SetType(TextureType);
			break;
		case D3D10_SVT_SAMPLER:
		case D3D10_SVT_SAMPLER1D:
		case D3D10_SVT_SAMPLER2D:
		case D3D10_SVT_SAMPLER3D:
		case D3D10_SVT_SAMPLERCUBE:
			this->SetType(SamplerType);
			break;
		}
	}
	else
	{
		this->SetType(UnknownType);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderVariable::Cleanup()
{
	n_assert(0 != this->d3d11Var);
	this->d3d11Var = 0;

	ShaderVariableBase::Cleanup();
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetInt(int value)
{
	this->d3d11Var->AsScalar()->SetInt(value);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetIntArray(const int* values, SizeT count)
{
	this->d3d11Var->AsScalar()->SetIntArray(values, 0, count);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetFloat(float value)
{
	this->d3d11Var->AsScalar()->SetFloat(value);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetFloatArray(const float* values, SizeT count)
{
	this->d3d11Var->AsScalar()->SetFloatArray(values, 0, count);
}


//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderVariable::SetFloat2( const Math::float2& value )
{
	this->d3d11Var->AsVector()->SetFloatVector((float*)&value);
}


//------------------------------------------------------------------------------
/**
*/
void 
D3D11ShaderVariable::SetFloat2Array( const Math::float2* values, SizeT count )
{
	this->d3d11Var->AsVector()->SetFloatVectorArray((float*)values, 0, count);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetFloat4(const float4& value)
{
	this->d3d11Var->AsVector()->SetFloatVector((float*)&value);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetFloat4Array(const float4* values, SizeT count)
{
	this->d3d11Var->AsVector()->SetFloatVectorArray((float*)values, 0, count);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetMatrix(const matrix44& value)
{
	this->d3d11Var->AsMatrix()->SetMatrix((float*)&value);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetMatrixArray(const matrix44* values, SizeT count)
{
	this->d3d11Var->AsMatrix()->SetMatrixArray((float*)values, 0, count);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetBool(bool value)
{
	this->d3d11Var->AsScalar()->SetBool((BOOL)value);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetBoolArray(const bool* values, SizeT count)
{
	// hmm... Win32's BOOL is actually an int
	const int MaxNumBools = 128;
	n_assert(count < MaxNumBools);
	BOOL tmp[MaxNumBools];
	IndexT i;
	for (i = 0; i < count; i++)
	{
		tmp[i] = (BOOL) values[i];
	}
	this->d3d11Var->AsScalar()->SetBoolArray(tmp, 0, count);
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11ShaderVariable::SetTexture(const Ptr<CoreGraphics::Texture>& value)
{
	if (value.isvalid() && value->IsLoaded())
	{
		n_assert(value->GetShaderResource());
		this->d3d11Var->AsShaderResource()->SetResource(value->GetShaderResource());
	}
	else
	{
		this->d3d11Var->AsShaderResource()->SetResource(0);
	}
	
}


} // namespace Direct3D11

