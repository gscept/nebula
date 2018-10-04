#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4StreamTextureLoader
  
    OGL4 implementation of StreamTextureLoader.

    (C) 2007 Radon Labs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "resources/streamresourceloader.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4StreamTextureLoader : public Resources::StreamResourceLoader
{
    __DeclareClass(OGL4StreamTextureLoader);
private:
    /// setup the texture from a Nebula stream
    virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};

} // namespace OpenGL4
//------------------------------------------------------------------------------