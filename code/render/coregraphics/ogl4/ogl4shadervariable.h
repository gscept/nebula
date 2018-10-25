#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4ShaderVariable
    
    OGL4 implementation of ShaderVariable.
    
    (C) 2013 Gustav Sterbrant
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "afxapi.h"
#include "coregraphics/base/shadervariablebase.h"
#include "util/variant.h"

//------------------------------------------------------------------------------

namespace CoreGraphics
{
class ConstantBuffer;
class Texture;
}

namespace OpenGL4
{
class OGL4ShaderInstance;
class OGL4StreamShaderLoader;
	
class OGL4ShaderVariable : public Base::ShaderVariableBase
{
    __DeclareClass(OGL4ShaderVariable);
public:
    /// constructor
    OGL4ShaderVariable();
    /// destructor
    virtual ~OGL4ShaderVariable();

    /// bind variable to uniform buffer
    void BindToUniformBuffer(const Ptr<CoreGraphics::ConstantBuffer>& buffer, GLuint offset, GLuint size, GLchar* defaultValue);
    
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
	/// sets buffer handle
	void SetBufferHandle(AnyFX::Handle* handle);

	/// cleanup
	void Cleanup();
private:
    friend class OpenGL4::OGL4StreamShaderLoader;
    friend class OpenGL4::OGL4ShaderInstance;

	/// setup from AnyFX variable
	void Setup(AnyFX::EffectVariable* var);
	/// setup from AnyFX varbuffer
	void Setup(AnyFX::EffectVarbuffer* var);
    /// setup from AnyFX varblock
    void Setup(AnyFX::EffectVarblock* var);

    struct BufferBinding
    {
        Ptr<CoreGraphics::ConstantBuffer> uniformBuffer;
        GLuint offset;
        GLuint size;
        GLchar* defaultValue;
    }* bufferBinding;
    
	Ptr<OGL4ShaderInstance> parentInstance;
	Ptr<CoreGraphics::Texture> texture;

	AnyFX::EffectVariable* effectVar;
	AnyFX::EffectVarbuffer* effectBuffer;
    AnyFX::EffectVarblock* effectBlock;
	bool reload;
};

} // namespace Direct3D9
//------------------------------------------------------------------------------
    