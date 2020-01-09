//------------------------------------------------------------------------------
//  helloworldrequesthandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/debug/helloworldrequesthandler.h"
#include "http/html/htmlpagewriter.h"

namespace Debug
{
__ImplementClass(Debug::HelloWorldRequestHandler, 'HWRH', Http::HttpRequestHandler);

using namespace Http;

//------------------------------------------------------------------------------
/**
*/
HelloWorldRequestHandler::HelloWorldRequestHandler()
{
    // we need to set a human-readable name, a description, and
    // the URI root location:
    this->SetName("Hello World");
    this->SetDesc("a sample HttpRequestHandler");
    this->SetRootLocation("helloworld");
}

//------------------------------------------------------------------------------
/**
*/
void
HelloWorldRequestHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    // we could extract more information from the URI if we want, but
    // since this is the most simple HttpRequestHandler possible, we wont :)

    // in order to server a valid HTML page to the request's ResponseContentStream, 
    // we need to create a HtmlPageWriter and connect it to the 
    // ResponseContentStream, this is standard Nebula IO stuff...
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Hello World");
    if (htmlWriter->Open())
    {
        // write some standard text to HTML page
        htmlWriter->Text("Hello World");
        htmlWriter->Close();

        // finally set the HTTP status code
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        // this shouldn't happen, but if something goes wrong, the 
        // web browser should know as well
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

} // namespace Debug
