#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11ShaderVariable
    
    D3D11 implementation of ShaderVariable.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/shadervariablebase.h"
#include "coregraphics/texture.h"
#include "util/variant.h"


namespace Direct3D11
{
class D3D11ShaderVariable : public Base::ShaderVariableBase
{
    __DeclareClass(D3D11ShaderVariable);
public:
    /// constructor
    D3D11ShaderVariable();
    /// destructor
    virtual ~D3D11ShaderVariable();
    
	/// set any value, this one needs size because we can't know the size of void
	void SetVoid(void* value, int size);
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

	/// cleanup
	void Cleanup();
private:
	friend class D3D11ShaderInstance;

	/// setup from D3D shader
	void Setup( ID3DX11EffectVariable* var);

	ID3DX11EffectVariable* d3d11Var;

	bool reload;
};


} // namespace Direct3D11
//------------------------------------------------------------------------------
    