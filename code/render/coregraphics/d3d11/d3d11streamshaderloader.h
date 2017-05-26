#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11StreamShaderLoader
    
    D3D11 implementation of StreamShaderLoader.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/streamresourceloader.h"


namespace Direct3D11
{
class D3D11StreamShaderLoader : public Resources::StreamResourceLoader
{
    __DeclareClass(D3D11StreamShaderLoader);
public:
    /// return true if asynchronous loading is supported
    virtual bool CanLoadAsync() const;
    
private:
    /// setup the shader from a Nebula3 stream
    virtual bool SetupResourceFromStream(const Ptr<IO::Stream>& stream);
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
    