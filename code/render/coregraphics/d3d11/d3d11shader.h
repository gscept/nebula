#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11Shader
    
    D3D11 implementation of Shader.

    @todo lost/reset device handling
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/blob.h"
#include "coregraphics/base/shaderbase.h"
#include "coregraphics/shaderstate.h"


namespace Direct3D11
{
	
class D3D11Shader : public Base::ShaderBase
{
    __DeclareClass(D3D11Shader);
public:

	
    /// constructor
    D3D11Shader();
    /// destructor
    virtual ~D3D11Shader();
   
    /// unload the resource, or cancel the pending load
    virtual void Unload();
	/// reloads a shader from file
	void Reload();
	
	/// returns pointer to effect
	ID3DX11Effect* GetD3D11Effect() const;
private:
    friend class D3D11StreamShaderLoader;

	/// cleans up the shader
	void Cleanup();

    /// called by d3d11 shader server when d3d11 device is lost
    void OnLostDevice();
    /// called by d3d11 shader server when d3d11 device is reset
    void OnResetDevice();

	ID3DX11Effect* effect;
};


//------------------------------------------------------------------------------
/**
*/
inline ID3DX11Effect* 
D3D11Shader::GetD3D11Effect() const
{
	return this->effect;
}

} // namespace Direct3D11
//------------------------------------------------------------------------------



    