#pragma once
//------------------------------------------------------------------------------
/**
    @class Materials::MaterialVariable
    
    Describes a material variable, similar to ShaderVariable.
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coregraphics/shadervariable.h"
#include "util/array.h"
#include "util/variant.h"
#include "coregraphics/texture.h"

//------------------------------------------------------------------------------
namespace Materials
{
class MaterialVariableInstance;
class MaterialVariable : public Core::RefCounted
{
	__DeclareClass(MaterialVariable);
public:

	/// material variable types
	enum Type
	{
		UnknownType,
		IntType,
		FloatType,
		VectorType,
		MatrixType,
		BoolType,
		TextureType,
		SamplerType
	};

	/// material variable typedef
	typedef Util::StringAtom Name;

	/// constructor
	MaterialVariable();
	/// destructor
	virtual ~MaterialVariable();

	/// applies variable settings
	void Apply();

	/// create a material variable instance
	Ptr<MaterialVariableInstance> CreateInstance();
	/// discard variable instance
	void DiscardVariableInstance(const Ptr<MaterialVariableInstance>& inst);

	/// setup material variable from list of shader variables
	void Setup(const Util::Array<Ptr<CoreGraphics::ShaderVariable>>& shaderVariables, Util::Variant defaultValue);
	/// cleans up the material variable
	void Cleanup();
	/// get the data type of the variable
	Type GetType() const;
	/// get the name of the variable
	const Name& GetName() const;

	/// set int value
	void SetInt(int value);
	/// set int array values
	void SetIntArray(const int* values, SizeT count);

	/// set float value
	void SetFloat(float value);
	/// set float array values
	void SetFloatArray(const float* values, SizeT count);

	/// set float 2 value
	void SetFloat2(const Math::float2& value);
	/// set float2 array value
	void SetFloat2Array(const Math::float2* values, SizeT count);

	/// set float4 value
	void SetFloat4(const Math::float4& value);
	/// set float4 array values
	void SetFloat4Array(const Math::float4* values, SizeT count);

	/// set matrix value
	void SetMatrix(const Math::matrix44& value);
	/// set matrix array values
	void SetMatrixArray(const Math::matrix44* values, SizeT count);

	/// set bool value
	void SetBool(bool value);
	/// set bool array values
	void SetBoolArray(const bool* values, SizeT count);

	/// set texture value
	void SetTexture(const Ptr<CoreGraphics::Texture>& value);

	/// gets a variant for the currently saved value
	const Util::Variant& GetValue() const;

	/// converts material variable type to util variant type
	static Util::Variant::Type VariantTypeFromType(MaterialVariable::Type type);

	/// gets the instances
	const Util::Array<Ptr<MaterialVariableInstance> >& GetInstances() const;

private:

	/// applies int
	void ApplyInt(int value);
	/// applies int array
	void ApplyIntArray(const int* values, SizeT count);
	/// applies float
	void ApplyFloat(float value);
	/// applies float array
	void ApplyFloatArray(const float* values, SizeT count);
	/// applies float2
	void ApplyFloat2(const Math::float2& value);
	/// applies float2 array
	void ApplyFloat2Array(const Math::float2* values, SizeT count);
	/// applies float4
	void ApplyFloat4(const Math::float4& value);
	/// applies float4 array
	void ApplyFloat4Array(const Math::float4* values, SizeT count);
	/// applies matrix
	void ApplyMatrix(const Math::matrix44& value);
	/// applies matrix array
	void ApplyMatrixArray(const Math::matrix44* values, SizeT count);
	/// applies boolean
	void ApplyBool(bool value);
	/// applies bool array
	void ApplyBoolArray(const bool* values, SizeT count);
	/// applies texture
	void ApplyTexture(const Ptr<CoreGraphics::Texture>& value);

	/// set variable type
	void SetType(Type t);
	/// set variable name
	void SetName(const Name& n);

	Type type;
	Name name;
	Util::Variant value;

	Util::Array<Ptr<CoreGraphics::ShaderVariable> > variables;
	Util::Array<Ptr<MaterialVariableInstance> > instances;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetType(MaterialVariable::Type t)
{
	this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline MaterialVariable::Type
MaterialVariable::GetType() const
{
	return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetName(const Name& n)
{
	this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const MaterialVariable::Name&
MaterialVariable::GetName() const
{
	return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Variant&
MaterialVariable::GetValue() const
{
	return this->value;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<MaterialVariableInstance> >& 
MaterialVariable::GetInstances() const
{
	return this->instances;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetInt(int value)
{
	this->value.SetInt(value);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetIntArray(const int* values, SizeT count)
{
	Util::Array<int> array;
	array.Reserve(count);
	for (int i = 0; i < count; i++)
	{
		array[i] = values[i];
	}
	this->value.SetIntArray(array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetFloat(float value)
{
	this->value.SetFloat(value);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetFloatArray(const float* values, SizeT count)
{
	Util::Array<float> array;
	array.Reserve(count);
	for (int i = 0; i < count; i++)
	{
		array[i] = values[i];
	}
	this->value.SetFloatArray(array);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MaterialVariable::SetFloat2( const Math::float2& value )
{
	this->value.SetFloat2(value);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
MaterialVariable::SetFloat2Array( const Math::float2* values, SizeT count )
{
	Util::Array<Math::float2> array;
	array.Reserve(count);
	for (int i = 0; i < count; i++)
	{
		array[i] = values[i];
	}
	this->value.SetFloat2Array(array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetFloat4(const Math::float4& value)
{
	this->value.SetFloat4(value);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetFloat4Array(const Math::float4* values, SizeT count)
{
	Util::Array<Math::float4> array;
	array.Reserve(count);
	for (int i = 0; i < count; i++)
	{
		array[i] = values[i];
	}
	this->value.SetFloat4Array(array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetMatrix(const Math::matrix44& value)
{
	this->value.SetMatrix44(value);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetMatrixArray(const Math::matrix44* values, SizeT count)
{
	Util::Array<Math::matrix44> array(count, 16);
	for (int i = 0; i < count; i++)
	{
		array.Append(values[i]);
	}
	this->value.SetMatrix44Array(array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetBool(bool value)
{
	this->value.SetBool(value);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetBoolArray(const bool* values, SizeT count)
{
	Util::Array<bool> array;
	array.Reserve(count);
	for (int i = 0; i < count; i++)
	{
		array[i] = values[i];
	}
	this->value.SetBoolArray(array);
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariable::SetTexture(const Ptr<CoreGraphics::Texture>& value)
{
	this->value.SetObject(value);
}

} // namespace Materials
//------------------------------------------------------------------------------
