#pragma once
//------------------------------------------------------------------------------
/**
    @class Materials::MaterialVariableInstance
    
    Describes a variable instance which holds the same information as a shader variable, 
	but only uses the information when Apply is called. Resembles the ShaderVariableInstance.
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/
#include "materials/materialvariable.h"
#include "core/refcounted.h"
#include "util/variant.h"

//------------------------------------------------------------------------------
namespace Materials
{
class MaterialVariable;
class MaterialVariableInstance : public Core::RefCounted
{
	__DeclareClass(MaterialVariableInstance);
public:
	/// constructor
	MaterialVariableInstance();
	/// destructor
	virtual ~MaterialVariableInstance();

	/// setup a material variable instance from a material variable
	void Setup(const Ptr<MaterialVariable>& var);
	/// discards a material instance
	void Discard();
	/// checks if the object is valid
	bool IsValid() const;

	/// prepare the variable for late-binding
	void Prepare(MaterialVariable::Type type);
	/// late-bind the variable instance to a shader variable
	void Bind(const Ptr<MaterialVariable>& var);
	/// get the associated material variable
	const Ptr<MaterialVariable>& GetMaterialVariable() const;
	/// apply local value to material variable
	void Apply();

	/// set int value
	void SetInt(int value);
	/// set float value
	void SetFloat(float value);
	/// set float2 value
	void SetFloat2(const Math::float2& value);
	/// set float4 value
	void SetFloat4(const Math::float4& value);
	/// set matrix44 value
	void SetMatrix(const Math::matrix44& value);
	/// set bool value
	void SetBool(bool value);
	/// set texture value
	void SetTexture(const Ptr<CoreGraphics::Texture>& value);
	/// set value directly
	void SetValue(const Util::Variant& v);

	/// get the value
	const Util::Variant& GetValue() const;

protected:
	friend class MaterialVariable;

	/// discards instance
	void Cleanup();

	Ptr<MaterialVariable> materialVariable;
	Util::Variant value;
}; 

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<MaterialVariable>&
MaterialVariableInstance::GetMaterialVariable() const
{
	return this->materialVariable;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetInt(int val)
{
	n_assert(this->value.GetType() == Util::Variant::Int);
	this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetFloat(float val)
{
	n_assert(this->value.GetType() == Util::Variant::Float);
	this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetFloat2(const Math::float2& val)
{
	n_assert(this->value.GetType() == Util::Variant::Float2);
	this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetFloat4(const Math::float4& val)
{
	n_assert(this->value.GetType() == Util::Variant::Float4);
	this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetMatrix(const Math::matrix44& val)
{
	n_assert(this->value.GetType() == Util::Variant::Matrix44);
	this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetBool(bool val)
{
	n_assert(this->value.GetType() == Util::Variant::Bool);
	this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetTexture(const Ptr<CoreGraphics::Texture>& value)
{
	n_assert(this->value.GetType() == Util::Variant::Object);
	this->value.SetObject((Core::RefCounted*)value.get());
}

//------------------------------------------------------------------------------
/**
*/
inline void
MaterialVariableInstance::SetValue(const Util::Variant& v)
{
	this->value = v;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Variant&
MaterialVariableInstance::GetValue() const
{
	return this->value;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
MaterialVariableInstance::IsValid() const
{
	return this->materialVariable.isvalid();
}

} // namespace Materials
//------------------------------------------------------------------------------