#pragma once
#ifndef DEBUG_SVGTESTPAGEHANDLER_H
#define DEBUG_SVGTESTPAGEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Debug::SvgTestPageHandler
    
    A HTTP test page handler to test SVG rendering functionality.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Debug
{
class SvgTestPageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(SvgTestPageHandler);
public:
    /// constructor
    SvgTestPageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request); 

private:
    /// test shape rendering
    bool TestShapeRendering(const Ptr<Http::HttpRequest>& request);
    /// test line chart rendering
    bool TestLineChartRendering(const Ptr<Http::HttpRequest>& request);
};

} // namespace Debug
//------------------------------------------------------------------------------
#endif

    