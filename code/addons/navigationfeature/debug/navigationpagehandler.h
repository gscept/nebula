#pragma once
//------------------------------------------------------------------------------
/**
    @class Navigation::NavigationPageHandler
    
    Provide information about the navigation system
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
#include "debug/debugpagehandler.h"

//------------------------------------------------------------------------------
namespace Navigation
{
class NavigationPageHandler : public Debug::DebugPageHandler
{
    __DeclareClass(NavigationPageHandler);
public:
    /// constructor
    NavigationPageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);

private: 
    /// handle HTTP request to render a counter char
    void HandleCounterChartRequest(const Util::String& counterName, const Ptr<Http::HttpRequest>& request);
};

} // namespace Navigation
//------------------------------------------------------------------------------  