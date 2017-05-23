#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::HelloWorldRequestHandler
  
    Most simple HttpRequestHandler possible. Invoke from web browser with

    http://127.0.0.1:2100/helloworld
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Debug
{
class HelloWorldRequestHandler : public Http::HttpRequestHandler
{
    __DeclareClass(HelloWorldRequestHandler);
public:
    /// constructor
    HelloWorldRequestHandler();
    /// handle a http request
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);
};

} // namespace Debug
//------------------------------------------------------------------------------
