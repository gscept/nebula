#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::IoPageHandler
    
    Provide information about IO subsystem to debug http server.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Debug
{
class IoPageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(IoPageHandler);
public:
    /// constructor
    IoPageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request); 

private:
    /// create a directory lister page
    void ListDirectory(const Util::String& path, const Ptr<Http::HttpRequest>& request);
};

} // namespace Debug
//------------------------------------------------------------------------------
    