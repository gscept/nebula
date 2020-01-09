#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::CorePageHandler
  
    Provide information about Core subsystem to debug http server.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Debug
{
class CorePageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(CorePageHandler);
public:
    /// constructor
    CorePageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);        
};

} // namespace Debug
//------------------------------------------------------------------------------
