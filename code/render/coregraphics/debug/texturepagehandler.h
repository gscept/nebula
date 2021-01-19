#pragma once
#ifndef DEBUG_TEXTUREPAGEHANDLER_H
#define DEBUG_TEXTUREPAGEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Debug::TexturePageHandler
    
    Provide a HTML debug page with information about all shared texture
    resources.
    
    Usage:    
    http://host/texture                         - provide a list of all textures with their properties
    http://host/texture?img=[resId]&fmt=[fmt]   - retrieve an image of the texture
    http://host/texture?texinfo=[resId]         - build a HTML page about specific texture    

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"
#include "resources/resourceid.h"

//------------------------------------------------------------------------------
namespace Debug
{
class TexturePageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(TexturePageHandler);
public:
    /// constructor
    TexturePageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);

private:
    /// handle a "raw" texture image request
    Http::HttpStatus::Code HandleImageRequest(const Util::Dictionary<Util::String,Util::String>& query, const Ptr<IO::Stream>& responseStream);
    /// handle a texture info request (returns an info page about a single texture)
    Http::HttpStatus::Code HandleTextureInfoRequest(const Util::String& resId, const Ptr<IO::Stream>& responseContentStream);
};

} // namespace Debug
//------------------------------------------------------------------------------
#endif
