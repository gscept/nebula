//------------------------------------------------------------------------------
//  ogl4shadervariableinstance.cc
//  (C) 2015-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4shadervariableinstance.h"

using namespace Math;
namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ShaderVariableInstance, 'O4VI', Base::ShaderVariableInstanceBase);

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderVariableInstance::OGL4ShaderVariableInstance() :
    bufferBinding(NULL)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderVariableInstance::~OGL4ShaderVariableInstance()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::BindToUniformBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buffer, GLuint offset, GLuint size, GLchar* defaultValue)
{
    this->bufferBinding = n_new(BufferBinding);
    this->bufferBinding->uniformBuffer = buffer;
    this->bufferBinding->offset = offset;
    this->bufferBinding->size = size;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::BindToUniformBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buffer, GLuint offset, GLuint size)
{
    this->bufferBinding = n_new(BufferBinding);
    this->bufferBinding->uniformBuffer = buffer;
    this->bufferBinding->offset = offset;
    this->bufferBinding->size = size;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::Cleanup()
{
	// unbind any handles
	if (this->bufferBinding)
	{
		n_delete(this->bufferBinding);
		this->bufferBinding = 0;
	}

	// unset handles
	if (this->shaderVariable->GetType() == CoreGraphics::ShaderVariable::BufferType)
	{
		this->shaderVariable->SetBufferHandle(NULL);
	}
	else if (this->shaderVariable->GetType() == CoreGraphics::ShaderVariable::TextureType)
	{
		this->shaderVariable->SetTexture(NULL);
	}
	ShaderVariableInstanceBase::Cleanup();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::Apply()
{
    n_assert(this->shaderVariable.isvalid());
    bool bufferBound = this->bufferBinding != NULL;
    switch (this->value.GetType())
    {
    case Util::Variant::VoidPtr:
        this->shaderVariable->SetBufferHandle((AnyFX::Handle*)this->value.GetVoidPtr());
        break;
    case Util::Variant::Object:
        // @note: implicit Ptr<> creation!
        if (this->value.GetObject() != 0)
        {
            this->shaderVariable->SetTexture((CoreGraphics::Texture*)this->value.GetObject());
        }
        break;
    default:
        if (!bufferBound) ShaderVariableInstanceBase::Apply();
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::ApplyTo(const Ptr<CoreGraphics::ShaderVariable>& var)
{
    n_assert(var.isvalid());
    bool bufferBound = this->bufferBinding != NULL;
    switch (this->value.GetType())
    {
    case Util::Variant::VoidPtr:
		var->SetBufferHandle((AnyFX::Handle*)this->value.GetVoidPtr());
        break;
    case Util::Variant::Object:
        // @note: implicit Ptr<> creation!
        if (this->value.GetObject() != 0)
        {
            var->SetTexture((CoreGraphics::Texture*)this->value.GetObject());
        }
        break;
    default:
        if (!bufferBound) ShaderVariableInstanceBase::Apply();
        break;
    }    
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetInt(int value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
        this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(int));
        break;
    case false:
        ShaderVariableInstanceBase::SetInt(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetIntArray(const int* values, SizeT count)
{
    n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(int), count);
        break;
    case false:
        ShaderVariableInstanceBase::SetIntArray(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetFloat(float value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(float));
        break;
    case false:
        ShaderVariableInstanceBase::SetFloat(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetFloatArray(const float* values, SizeT count)
{
    n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(float), count);
        break;
    case false:
        ShaderVariableInstanceBase::SetFloatArray(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetFloat2(const Math::float2& value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(Math::float2));
        break;
    case false:
        ShaderVariableInstanceBase::SetFloat2(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetFloat2Array(const Math::float2* values, SizeT count)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(Math::float2), count);
        break;
    case false:
        ShaderVariableInstanceBase::SetFloat2Array(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetFloat4(const float4& value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(Math::float4));
        break;
    case false:
        ShaderVariableInstanceBase::SetFloat4(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetFloat4Array(const float4* values, SizeT count)
{
    n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(Math::float4), count);
        break;
    case false:
        ShaderVariableInstanceBase::SetFloat4Array(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetMatrix(const matrix44& value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(Math::matrix44));
        break;
    case false:
        ShaderVariableInstanceBase::SetMatrix(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetMatrixArray(const matrix44* values, SizeT count)
{
    n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(Math::matrix44), count);
        break;
    case false:
        ShaderVariableInstanceBase::SetMatrixArray(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetBool(bool value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
        this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(bool));
        break;
    case false:
        ShaderVariableInstanceBase::SetBool(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetBoolArray(const bool* values, SizeT count)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(bool), count);
        break;
    case false:
        ShaderVariableInstanceBase::SetBoolArray(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetValue(const Util::Variant& v)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
        switch (v.GetType())
        {
        case Util::Variant::Int:
			this->bufferBinding->uniformBuffer->Update(v.GetInt(), this->bufferBinding->offset, sizeof(int));
            break;
        case Util::Variant::UInt:
			this->bufferBinding->uniformBuffer->Update(v.GetUInt(), this->bufferBinding->offset, sizeof(uint));
            break;
        case Util::Variant::Float:
			this->bufferBinding->uniformBuffer->Update(v.GetFloat(), this->bufferBinding->offset, sizeof(float));
            break;
        case Util::Variant::Bool:
            this->bufferBinding->uniformBuffer->Update(v.GetBool(), this->bufferBinding->offset, sizeof(bool));
            break;
        case Util::Variant::Float2:
			this->bufferBinding->uniformBuffer->Update(v.GetFloat2(), this->bufferBinding->offset, sizeof(Math::float2));
            break;
        case Util::Variant::Float4:
			this->bufferBinding->uniformBuffer->Update(v.GetFloat4(), this->bufferBinding->offset, sizeof(Math::float4));
            break;
        case Util::Variant::Matrix44:
			this->bufferBinding->uniformBuffer->Update(v.GetMatrix44(), this->bufferBinding->offset, sizeof(Math::matrix44));
            break;
        case Util::Variant::IntArray:
        {
            const Util::Array<int>& arr = v.GetIntArray();
            this->bufferBinding->uniformBuffer->UpdateArray(&arr[0], this->bufferBinding->offset, sizeof(int), arr.Size());
            break;
        }
        case Util::Variant::FloatArray:
        {
            const Util::Array<float>& arr = v.GetFloatArray();
			this->bufferBinding->uniformBuffer->UpdateArray(&arr[0], this->bufferBinding->offset, sizeof(float), arr.Size());
            break;
        }
        case Util::Variant::BoolArray:
        {
            const Util::Array<bool>& arr = v.GetBoolArray();
            this->bufferBinding->uniformBuffer->UpdateArray(&arr[0], this->bufferBinding->offset, sizeof(bool), arr.Size());
            break;
        }
        case Util::Variant::Float2Array:
        {
            const Util::Array<Math::float2>& arr = v.GetFloat2Array();
            this->bufferBinding->uniformBuffer->UpdateArray(&arr[0], this->bufferBinding->offset, sizeof(Math::float2), arr.Size());
            break;
        }
        case Util::Variant::Float4Array:
        {
            const Util::Array<Math::float4>& arr = v.GetFloat4Array();
			this->bufferBinding->uniformBuffer->UpdateArray(&arr[0], this->bufferBinding->offset, sizeof(Math::float4), arr.Size());
            break;
        }
        case Util::Variant::Matrix44Array:
        {
            const Util::Array<Math::matrix44>& arr = v.GetMatrix44Array();
			this->bufferBinding->uniformBuffer->UpdateArray(&arr[0], this->bufferBinding->offset, sizeof(Math::matrix44), arr.Size());
            break;
        }
        }
        break;
    case false:
        ShaderVariableInstanceBase::SetValue(v);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariableInstance::SetBufferHandle(void* handle)
{
    this->value.SetType(Util::Variant::VoidPtr);
    this->value.SetVoidPtr(handle);
}


} // namespace OpenGL4