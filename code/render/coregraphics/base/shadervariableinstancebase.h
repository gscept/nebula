#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ShaderVariableInstanceBase
    
    A ShaderVariableInstance associates a value with a ShaderVariable
    and can apply that value at any time to the ShaderVariable.
    Setting the value on a ShaderVariableInstance will just store the
    value but not change the actual ShaderVariable. Only calling
    Apply() will set the stored value on the ShaderVariable. 
    ShaderVariableInstance objects are used to manage per-instance
    state when rendering ModelNodeInstances.

    NOTE: you cannot set arrays through a ShaderVariableInstance!
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/variant.h"
#include "coregraphics/texture.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include "coregraphics/texture.h"
#include "coregraphics/shadervariable.h"

//------------------------------------------------------------------------------
namespace Base
{
class ShaderVariableInstanceBase : public Core::RefCounted
{
    __DeclareClass(ShaderVariableInstanceBase);
public:
    /// constructor
    ShaderVariableInstanceBase();
    /// destructor
    virtual ~ShaderVariableInstanceBase();

    /// setup the object from a shader variable
    void Setup(const Ptr<CoreGraphics::ShaderVariable>& var);
	/// setup the object from just a variable type, this will only be valid to use with ApplyTo
	void Setup(const Base::ShaderVariableBase::Type& type);
	/// discards instance
	void Discard();

    /// get the associated shader variable 
    const Ptr<CoreGraphics::ShaderVariable>& ShaderStateGetVariable() const;
    /// apply local value to shader variable
    void Apply();
	/// apply local value to specific shader variable
	void ApplyTo(const Ptr<CoreGraphics::ShaderVariable>& var);

    /// set int value
    void SetInt(int value);
    /// set int array values
    void SetIntArray(const int* values, SizeT count);
    /// set float value
    void SetFloat(float value);
    /// set float array values
    void SetFloatArray(const float* values, SizeT count);
    /// set vector value
    void SetFloat2(const Math::float2& value);
    /// set vector array values
    void SetFloat2Array(const Math::float2* values, SizeT count);
    /// set float4 value
    void SetFloat4(const Math::float4& value);
    /// set vector array values
    void SetFloat4Array(const Math::float4* values, SizeT count);
    /// set matrix44 value
    void SetMatrix(const Math::matrix44& value);
    /// set matrix array values
    void SetMatrixArray(const Math::matrix44* values, SizeT count);
    /// set bool value
    void SetBool(bool value);
    /// set bool array values
    void SetBoolArray(const bool* values, SizeT count);
    /// set texture value
    void SetTexture(const Ptr<CoreGraphics::Texture>& value);
	/// set constant buffer
	void SetConstantBuffer(const Ptr<CoreGraphics::ConstantBuffer>& value);
	/// set shader read-write texture
	void SetShaderReadWriteTexture(const Ptr<CoreGraphics::ShaderReadWriteTexture>& value);
	/// set shader read-write texture
	void SetShaderReadWriteTexture(const Ptr<CoreGraphics::Texture>& value);
	/// set shader read-write buffer
	void SetShaderReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& value);

	/// set textures which may be bound later
    void SetDeferredTexture(const Util::String& name);
    /// set value directly
    void SetValue(const Util::Variant& v);

	/// get the value
	const Util::Variant& GetValue() const;

protected:
	friend class ShaderVariableBase;

	enum ObjectType
	{
		TextureObjectType,
		ReadWriteImageObjectType,

		ConstantBufferObjectType,
		ReadWriteBufferObjectType,

		NoObjectType
	};

	/// cleans up instance
	virtual void Cleanup();

	/// prepare the object for late-binding
	void Prepare(CoreGraphics::ShaderVariable::Type type);

    Ptr<CoreGraphics::ShaderVariable> shaderVariable;
    Util::Variant value;        // for scalar values
	ObjectType type;
	Util::String deferredTexture;
};

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderVariable>&
ShaderVariableInstanceBase::ShaderStateGetVariable() const
{
    return this->shaderVariable;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetInt(int val)
{
    n_assert(this->value.GetType() == Util::Variant::Int);
    this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetIntArray(const int* values, SizeT count)
{
    n_assert(this->value.GetType() == Util::Variant::IntArray);
    Util::Array<int> arr;
    arr.Reserve(count);
    IndexT i;
    for (i = 0; i < arr.Size(); i++) arr[i] = values[i];
    this->value.SetIntArray(arr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetFloat(float val)
{
    n_assert(this->value.GetType() == Util::Variant::Float);
    this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetFloatArray(const float* values, SizeT count)
{
    n_assert(this->value.GetType() == Util::Variant::FloatArray);
    Util::Array<float> arr;
    arr.Reserve(count);
    IndexT i;
    for (i = 0; i < arr.Size(); i++) arr[i] = values[i];
    this->value.SetFloatArray(arr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetFloat2(const Math::float2& value)
{
    n_assert(this->value.GetType() == Util::Variant::Float2);
    this->value.SetFloat2(value);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetFloat2Array(const Math::float2* values, SizeT count)
{
    n_assert(this->value.GetType() == Util::Variant::Float2Array);
    Util::Array<Math::float2> arr;
    arr.Reserve(count);
    IndexT i;
    for (i = 0; i < arr.Size(); i++) arr[i] = values[i];
    this->value.SetFloat2Array(arr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetFloat4(const Math::float4& val)
{
    n_assert(this->value.GetType() == Util::Variant::Float4);
    this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetFloat4Array(const Math::float4* values, SizeT count)
{
    n_assert(this->value.GetType() == Util::Variant::Float4Array);
    Util::Array<Math::float4> arr;
    arr.Reserve(count);
    IndexT i;
    for (i = 0; i < arr.Size(); i++) arr[i] = values[i];
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetMatrix(const Math::matrix44& val)
{
    n_assert(this->value.GetType() == Util::Variant::Matrix44);
    this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetMatrixArray(const Math::matrix44* values, SizeT count)
{
    n_assert(this->value.GetType() == Util::Variant::Matrix44Array);
    Util::Array<Math::matrix44> arr;
    arr.Reserve(count);
    IndexT i;
    for (i = 0; i < arr.Size(); i++) arr[i] = values[i];
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetBool(bool val)
{
    n_assert(this->value.GetType() == Util::Variant::Bool);
    this->value = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetBoolArray(const bool* values, SizeT count)
{
    n_assert(this->value.GetType() == Util::Variant::BoolArray);
    Util::Array<bool> arr;
    arr.Reserve(count);
    IndexT i;
    for (i = 0; i < arr.Size(); i++) arr[i] = values[i];
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetTexture(const Ptr<CoreGraphics::Texture>& value)
{
    n_assert(this->value.GetType() == Util::Variant::Object);
    this->value.SetObject((Core::RefCounted*)value.get());
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetDeferredTexture(const Util::String& name)
{
	n_assert(this->value.GetType() == Util::Variant::Object);
	this->deferredTexture = name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetConstantBuffer(const Ptr<CoreGraphics::ConstantBuffer>& value)
{
	n_assert(this->value.GetType() == Util::Variant::Object);
	this->value.SetObject((Core::RefCounted*)value.get());
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetShaderReadWriteTexture(const Ptr<CoreGraphics::ShaderReadWriteTexture>& value)
{
	n_assert(this->value.GetType() == Util::Variant::Object);
	this->value.SetObject((Core::RefCounted*)value.get());
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetShaderReadWriteTexture(const Ptr<CoreGraphics::Texture>& value)
{
	n_assert(this->value.GetType() == Util::Variant::Object);
	this->value.SetObject((Core::RefCounted*)value.get());
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetShaderReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& value)
{
	n_assert(this->value.GetType() == Util::Variant::Object);
	this->value.SetObject((Core::RefCounted*)value.get());
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableInstanceBase::SetValue(const Util::Variant& v)
{
    n_assert(value.GetType() == v.GetType());
    this->value = v;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Variant&
ShaderVariableInstanceBase::GetValue() const
{
	return this->value;
}

} // namespace Base
//------------------------------------------------------------------------------



