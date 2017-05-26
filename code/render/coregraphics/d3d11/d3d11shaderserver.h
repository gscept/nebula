#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11ShaderServer
    
    D3D11 implementation of ShaderServer.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/shaderserverbase.h"

namespace Direct3D11
{


class D3D11ShaderServer : public Base::ShaderServerBase
{
    __DeclareClass(D3D11ShaderServer);
    __DeclareSingleton(D3D11ShaderServer);
public:
    /// constructor
    D3D11ShaderServer();
    /// destructor
    virtual ~D3D11ShaderServer();
    
    /// open the shader server
    bool Open();
    /// close the shader server
    void Close();

	/// reloads a shader
	void ReloadShader(Ptr<CoreGraphics::Shader> shader);
	/// explicitly loads a shader by resource id
	void LoadShader(const Resources::ResourceId& shdName);
};



} // namespace Direct3D11
//------------------------------------------------------------------------------
    