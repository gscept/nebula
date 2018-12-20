//------------------------------------------------------------------------------
//  OGL4ShaderVariable.cc
//  (C) 2013 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/texture.h"
#include "coregraphics/ogl4/ogl4shadervariable.h"
#include "coregraphics/shadervariableinstance.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "math/vector.h"
#include "ogl4uniformbuffer.h"


namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4ShaderVariable, 'D1VR', Base::ShaderVariableBase);

using namespace CoreGraphics;
using namespace Math;
using namespace Util;
//------------------------------------------------------------------------------
/**
*/
OGL4ShaderVariable::OGL4ShaderVariable() :	
    effectVar(0),
    effectBuffer(0),
    effectBlock(0),
    bufferBinding(NULL),
	texture(0)
{
    // empty
}    

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderVariable::~OGL4ShaderVariable()
{
	this->texture = 0;
	this->effectVar = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::BindToUniformBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buffer, GLuint offset, GLuint size, GLchar* defaultValue)
{
    this->bufferBinding = new BufferBinding;
    this->bufferBinding->uniformBuffer = buffer;
    this->bufferBinding->offset = offset;
    this->bufferBinding->size = size;
    this->bufferBinding->defaultValue = defaultValue;

    // make sure that the buffer is updated (data is array since we have a char*)
    buffer->UpdateArray(defaultValue, offset, size, 1);
}

//------------------------------------------------------------------------------
/**
*/
void OGL4ShaderVariable::Setup(AnyFX::EffectVariable* var)
{
	n_assert(0 == effectVar);
	
	String name = var->GetName().c_str();
	this->SetName(name);
	switch (var->GetType())
	{
	case AnyFX::Double:
	case AnyFX::Float:
		this->SetType(FloatType);
		break;
	case AnyFX::Short:
	case AnyFX::Integer:
	case AnyFX::UInteger:
		this->SetType(IntType);
		break;
	case AnyFX::Bool:
		this->SetType(BoolType);
		break;
	case AnyFX::Float3:
	case AnyFX::Float4:	
	case AnyFX::Double3:
	case AnyFX::Double4:
	case AnyFX::Integer3:
	case AnyFX::Integer4:
	case AnyFX::UInteger3:
	case AnyFX::UInteger4:
	case AnyFX::Short3:
	case AnyFX::Short4:
	case AnyFX::Bool3:
	case AnyFX::Bool4:
		this->SetType(VectorType);
		break;
	case AnyFX::Float2:
	case AnyFX::Double2:
	case AnyFX::Integer2:
	case AnyFX::UInteger2:
	case AnyFX::Short2:
	case AnyFX::Bool2:
		this->SetType(Vector2Type);
		break;
	case AnyFX::Matrix2x2:
	case AnyFX::Matrix2x3:
	case AnyFX::Matrix2x4:
	case AnyFX::Matrix3x2:
	case AnyFX::Matrix3x3:
	case AnyFX::Matrix3x4:
	case AnyFX::Matrix4x2:
	case AnyFX::Matrix4x3:
	case AnyFX::Matrix4x4:
		this->SetType(MatrixType);
		break;
	case AnyFX::Sampler1D:
	case AnyFX::Sampler1DArray:
	case AnyFX::Sampler2D:
	case AnyFX::Sampler2DArray:
	case AnyFX::Sampler2DMS:
	case AnyFX::Sampler2DMSArray:
	case AnyFX::Sampler3D:
	case AnyFX::SamplerCube:
	case AnyFX::SamplerCubeArray:
	case AnyFX::Image1D:
	case AnyFX::Image1DArray:
	case AnyFX::Image2D:
	case AnyFX::Image2DArray:
	case AnyFX::Image2DMS:
	case AnyFX::Image2DMSArray:
	case AnyFX::Image3D:
	case AnyFX::ImageCube:
	case AnyFX::ImageCubeArray:
		this->SetType(TextureType);
		break;
	}
	this->effectVar = var;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::Setup(AnyFX::EffectVarbuffer* var)
{
	String name = var->GetName().c_str();
	this->SetName(name);
	this->effectBuffer = var;
	this->SetType(BufferType);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::Setup(AnyFX::EffectVarblock* var)
{
    Util::String name = var->GetName().c_str();
    this->SetName(name);
    this->effectBlock = var;
    this->SetType(BufferType);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4ShaderVariable::Cleanup()
{
	ShaderVariableBase::Cleanup();
	if (this->bufferBinding != NULL)
	{
		delete this->bufferBinding;
	}
	if (this->effectBuffer != NULL)
	{
		this->effectBuffer->SetBuffer(NULL);
		this->effectBuffer = 0;
	}
	if (this->effectBlock != NULL)
	{
		this->effectBlock->SetBuffer(NULL);
		this->effectBlock = 0;
	}
	this->effectVar = 0;
	this->texture = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetInt(int value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(int));
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetInt(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetIntArray(const int* values, SizeT count)
{
	n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(int), count);
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetIntArray(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetFloat(float value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(float));
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetFloat(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetFloatArray(const float* values, SizeT count)
{
	n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
        this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(float), count);
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetFloatArray(values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetFloat2(const float2& value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(float2));
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetFloat2((float*)&value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetFloat2Array(const float2* values, SizeT count)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(float2), count);
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetFloat2Array((float*)&values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetFloat4(const float4& value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(float4));
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetFloat4((float*)&value);
        break;
    }	
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetFloat4Array(const float4* values, SizeT count)
{
	n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(float4), count);
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetFloat4Array((float*)values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetMatrix(const matrix44& value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(matrix44));
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetMatrix((float*)&value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetMatrixArray(const matrix44* values, SizeT count)
{
	n_assert(0 != values);
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		// hmm, the buffer binding size will already be with the valid array size...
        this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(matrix44), count);
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetMatrixArray((float*)values, count);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetBool(bool value)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
        this->bufferBinding->uniformBuffer->Update(value, this->bufferBinding->offset, sizeof(bool));
        break;
    case false:
        n_assert(0 != this->effectVar);
        this->effectVar->SetBool(value);
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetBoolArray(const bool* values, SizeT count)
{
    bool bufferBound = this->bufferBinding != NULL;
    switch (bufferBound)
    {
    case true:
		this->bufferBinding->uniformBuffer->UpdateArray(values, this->bufferBinding->offset, sizeof(bool), count);
        break;
    case false:
        n_assert(0 != this->effectVar);
		this->effectVar->SetBoolArray(values, count);
        break;
    }
	
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetTexture(const Ptr<CoreGraphics::Texture>& value)
{
	n_assert(0 != this->effectVar);
	this->texture = value;
    if (value.isvalid())
    {
		this->effectVar->SetTexture((AnyFX::Handle*)value->GetOGL4Variable());
    }
    else
    {
        this->effectVar->SetTexture(NULL);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderVariable::SetBufferHandle(AnyFX::Handle* handle)
{
	n_assert(0 != this->effectBuffer || 0 != this->effectBlock);
    if (this->effectBlock)  this->effectBlock->SetBuffer(handle);
	else                    this->effectBuffer->SetBuffer(handle);
}

} // namespace OpenGL4
