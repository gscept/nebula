#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D9::OGL4ShaderServer
    
    OGL4 implementation of ShaderServer.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "coregraphics/base/shaderserverbase.h"
#include "coregraphics/ogl4/ogl4shader.h"
#include "effectfactory.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4ShaderServer : public Base::ShaderServerBase
{
    __DeclareClass(OGL4ShaderServer);
    __DeclareSingleton(OGL4ShaderServer);
public:
    /// constructor
    OGL4ShaderServer();
    /// destructor
    virtual ~OGL4ShaderServer();
    
    /// open the shader server
    bool Open();
    /// close the shader server
    void Close();

private:
	AnyFX::EffectFactory* factory;
};

} // namespace Direct3D9
//------------------------------------------------------------------------------
    