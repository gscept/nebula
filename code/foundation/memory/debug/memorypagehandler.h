#pragma once
#ifndef DEBUG_MEMORYPAGEHANDLER_H
#define DEBUG_MEMORYPAGEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Debug::MemoryPageHandler
  
    Provide information about memory allocations to debug http server.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Debug
{
class MemoryPageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(MemoryPageHandler);
public:
    /// constructor
    MemoryPageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);
};

} // namespace Debug
//------------------------------------------------------------------------------
#endif
