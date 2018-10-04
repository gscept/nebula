#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D9::OGL4StreamShaderLoader
    
    OGL4 implementation of StreamShaderLoader.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/streamresourceloader.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4StreamShaderLoader : public Resources::StreamResourceLoader
{
    __DeclareClass(OGL4StreamShaderLoader);
public:
    /// return true if asynchronous loading is supported
    virtual bool CanLoadAsync() const;
    
private:
    /// setup the shader from a Nebula stream
    virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};

} // namespace OpenGL4
//------------------------------------------------------------------------------
    