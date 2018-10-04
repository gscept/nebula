#pragma once
#ifndef DEBUG_DISPLAYPAGEHANDLER_H
#define DEBUG_DISPLAYPAGEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Debug::DisplayPageHandler
    
    Provide information about the display to the debug http server.
    
    The DisplayPageHandler can also serve a screenshot:

    http://host/display/screenshot?fmt=[format]

    Where format is one of jpg,bmp,png.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"
#include "coregraphics/adapter.h"
#include "http/html/htmlpagewriter.h"

//------------------------------------------------------------------------------
namespace Debug
{
class DisplayPageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(DisplayPageHandler);
public:
    /// constructor
    DisplayPageHandler();
    /// handle a http request, the handler is expected to fill the content stream with response data
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);

private:
    /// write info about an adapter
    void WriteAdapterInfo(CoreGraphics::Adapter::Code adapter, const Ptr<Http::HtmlPageWriter>& htmlWriter);
    /// write a screenshot
    Http::HttpStatus::Code WriteScreenshot(const Util::String& fileFormat, const Ptr<IO::Stream>& responseContentStream);
};

} // namespace Debug
//------------------------------------------------------------------------------
#endif
    