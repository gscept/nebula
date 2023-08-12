//------------------------------------------------------------------------------
//  threadpagehandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "threading/debug/threadpagehandler.h"
#include "threading/thread.h"
#include "http/html/htmlpagewriter.h"

namespace Debug
{
__ImplementClass(Debug::ThreadPageHandler, 'TPGH', Http::HttpRequestHandler);

using namespace IO;
using namespace Http;
using namespace Util;
using namespace Threading;

//------------------------------------------------------------------------------
/**
*/
ThreadPageHandler::ThreadPageHandler()
{
    this->SetName("Threads");
    this->SetDesc("display running threads");
    this->SetRootLocation("thread");
}

//------------------------------------------------------------------------------
/**
*/
void
ThreadPageHandler::HandleRequest(const Ptr<HttpRequest>& request) 
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // configure a HTML page writer
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Thread Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Nebula Threads");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");

        // if not compiled with NEBULA_DEBUG, display a message
        #if (NEBULA_DEBUG == 0)
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();
        htmlWriter->Text("Thread list not available because application was not compiled with debug information!");
        #else
        htmlWriter->Element(HtmlElement::Heading3, "Running Threads");
        Array<Thread::ThreadDebugInfo> threadInfos = Thread::GetRunningThreadDebugInfos();

        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "Thread Name");
                htmlWriter->Element(HtmlElement::TableHeader, "Priority");
                htmlWriter->Element(HtmlElement::TableHeader, "Affinity mask");
                htmlWriter->Element(HtmlElement::TableHeader, "Stack Size");
                htmlWriter->Element(HtmlElement::TableHeader, "CPU Cores");
            htmlWriter->End(HtmlElement::TableRow);

            IndexT i;
            for (i = 0; i < threadInfos.Size(); i++)
            {
                const Thread::ThreadDebugInfo& curThreadInfo = threadInfos[i];
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, curThreadInfo.threadName);
                    String pri = "UNKNOWN";
                    switch (curThreadInfo.threadPriority)
                    {
                        case Thread::Low:       pri = "Low"; break;
                        case Thread::Normal:    pri = "Normal"; break;
                        case Thread::High:      pri = "High"; break;
                    }

                    String cores = "";
                    for (SizeT i = 0; i < 32; i++)
                    {
                        uint bit = 1 << i;
                        if (((uint)curThreadInfo.threadCoreId & bit) == bit)
                        {
                            String coreStr;
                            coreStr.Format("Core%d | ", i);
                            cores.Append(coreStr);
                        }
                    }

                    htmlWriter->Element(HtmlElement::TableData, pri);
                    htmlWriter->Element(HtmlElement::TableData, String::Hex(curThreadInfo.threadCoreId));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(curThreadInfo.threadStackSize));
                    htmlWriter->Element(HtmlElement::TableData, cores);
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);
        #endif
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

} // namespace Debug
