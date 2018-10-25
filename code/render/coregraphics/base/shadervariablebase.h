#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::ShaderVariableBase
    
    Provides direct access to a shader's global variable.
    The fastest way to change the value of a shader variable is to
    obtain a pointer to a shader variable once, and use it repeatedly
    to set new values.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "util/array.h"
#include "util/variant.h"

namespace CoreGraphics
{
class Texture;
class ShaderState;
class ShaderVariableInstance;
class ConstantBuffer;
class ShaderReadWriteTexture;
class ShaderReadWriteBuffer;
}

//------------------------------------------------------------------------------
namespace Base
{
class ShaderVariableBase : public Core::RefCounted
{
    __DeclareClass(ShaderVariableBase);
public:
    /// shader variable types
    enum Type
    {
        UnknownType,
        IntType,
        FloatType,
        VectorType,		// float4
		Vector2Type,	// float2
        MatrixType,
        BoolType,
        TextureType,
		SamplerType,
		ConstantBufferType,
		ImageReadWriteType,
		BufferReadWriteType
    };

    /// shader variable name typedef
    typedef Util::StringAtom Name;

    /// constructor
    ShaderVariableBase();
    /// destructor
    virtual ~ShaderVariableBase();

    /// create a shader variable instance
    Ptr<CoreGraphics::ShaderVariableInstance> CreateInstance();
	/// removes shader variable instance
	void DiscardInstance(const Ptr<CoreGraphics::ShaderVariableInstance>& inst);

    /// get the data type of the variable
    Type GetType() const;
    /// get the name of the variable
    const Name& GetName() const;
	/// get the default value of the variable
	const Util::Variant& GetDefaultValue() const;

    /// convert type to string
    static Util::String TypeToString(Type t);
    /// convert string to type
    static Type StringToType(const Util::String& str);

    /// set int value
    void SetInt(int value);
    /// set int array values
    void SetIntArray(const int* values, SizeT count);

    /// set float value
    void SetFloat(float value);
    /// set float array values
    void SetFloatArray(const float* values, SizeT count);

	/// set vector vector value
	void SetFloat2(const Math::float2& value);
		/// set vector array values
	void SetFloat2Array(const Math::float2* values, SizeT count);
	
    /// set vector value
    void SetFloat4(const Math::float4& value);	
    /// set vector array values
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
	/// set constant buffer
	void SetConstantBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buf);
	/// set shader read-write image
	void SetShaderReadWriteTexture(const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex);
	/// set shader read-write as texture
	void SetShaderReadWriteTexture(const Ptr<CoreGraphics::Texture>& tex);
	/// set shader read-write buffer
	void SetShaderReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf);

	/// reloads all variable instances
	virtual void Reload();
	/// returns true if this variable is active in the shader state, implement for subclass
	const bool IsActive() const;

	/// removes all instances
	virtual void Cleanup();

protected:
	friend class ShaderVariableInstanceBase;

    /// set variable type
    void SetType(Type t);
    /// set variable name
    void SetName(const Name& n);

    Type type;
    Name name;
	Util::Variant defaultValue;

	Util::Array<Ptr<CoreGraphics::ShaderVariableInstance> > instances;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableBase::SetType(Type t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline ShaderVariableBase::Type
ShaderVariableBase::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShaderVariableBase::SetName(const Name& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const ShaderVariableBase::Name&
ShaderVariableBase::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Variant&
ShaderVariableBase::GetDefaultValue() const
{
	return this->defaultValue;
}

} // Base
//------------------------------------------------------------------------------
 