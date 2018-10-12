#pragma once
#ifndef HTTP_DEFAULTHTTPREQUESTHANDLER_H
#define HTTP_DEFAULTHTTPREQUESTHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Http::DefaultHttpRequestHandler
    
    This implements the "Home Page" of the Nebula application. It will
    answer all HTTP requests which are not handled by a custom
    HttpRequestHandler.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Http
{
class DefaultHttpRequestHandler : public HttpRequestHandler
{
    __DeclareClass(DefaultHttpRequestHandler);
public:
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<HttpRequest>& request);
};

} // namespace HttpRequestHandler
//------------------------------------------------------------------------------
#endif
    