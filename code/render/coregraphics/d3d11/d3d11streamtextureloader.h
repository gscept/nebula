#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11StreamTextureLoader
  
    D3D11/Xbox360 implementation of StreamTextureLoader.

    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "resources/streamresourceloader.h"


namespace Direct3D11
{
class D3D11StreamTextureLoader : public Resources::StreamResourceLoader
{
    __DeclareClass(D3D11StreamTextureLoader);
private:
    /// setup the texture from a Nebula3 stream
    virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};

} // namespace Direct3D11
//------------------------------------------------------------------------------