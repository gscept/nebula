#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::ConsolePageHandler
    
    Print console output to HTML page.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"    
#include "io/historyconsolehandler.h"

//------------------------------------------------------------------------------
namespace Debug
{
class ConsolePageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(ConsolePageHandler);
public:
    /// constructor
    ConsolePageHandler();
    /// destructor
    virtual ~ConsolePageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request); 

private:
    Ptr<IO::HistoryConsoleHandler> historyConsoleHandler;
};

} // namespace Debug
//------------------------------------------------------------------------------
