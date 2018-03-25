//------------------------------------------------------------------------------
//  consolepagehandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/debug/consolepagehandler.h"
#include "io/console.h"
#include "http/html/htmlpagewriter.h"

namespace Debug
{
__ImplementClass(Debug::ConsolePageHandler, 'COPH', Http::HttpRequestHandler);

using namespace Http;
using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ConsolePageHandler::ConsolePageHandler()
{
    // setup request handler
    this->SetName("Console");
    this->SetDesc("show console log");
    this->SetRootLocation("console");

    // attach a history console handler which keeps track of log messages
    this->historyConsoleHandler = HistoryConsoleHandler::Create();
    Console::Instance()->AttachHandler(this->historyConsoleHandler.upcast<ConsoleHandler>());
}

//------------------------------------------------------------------------------
/**
*/
ConsolePageHandler::~ConsolePageHandler()
{
    this->historyConsoleHandler = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
ConsolePageHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Console Log");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Console Log");

        // get current console log history
        const RingBuffer<String>& history = this->historyConsoleHandler->GetHistory();
        SizeT num = history.Size();
        IndexT i;
        for (i = 0; i < num; i++)
        {
            htmlWriter->Text(history[i]);
            htmlWriter->LineBreak();
        }
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

} // namespace IO
