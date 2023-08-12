//------------------------------------------------------------------------------
//  corepagehandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "core/debug/corepagehandler.h"
#include "http/html/htmlpagewriter.h"
#include "system/systeminfo.h"
#include "core/refcounted.h"

namespace Debug
{
__ImplementClass(Debug::CorePageHandler, 'CPGH', Http::HttpRequestHandler);

using namespace IO;
using namespace Http;
using namespace Util;
using namespace Core;
using namespace System;

//------------------------------------------------------------------------------
/**
*/
CorePageHandler::CorePageHandler()
{
    this->SetName("Core");
    this->SetDesc("show debug information about Core subsystem");
    this->SetRootLocation("core");
}

//------------------------------------------------------------------------------
/**
*/
void
CorePageHandler::HandleRequest(const Ptr<HttpRequest>& request) 
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // configure a HTML page writer
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Core Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Core Subsystem");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");

        // write basic system info
        htmlWriter->Element(HtmlElement::Heading3, "System Info");
        SystemInfo systemInfo;
        htmlWriter->Begin(HtmlElement::Table);        
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Host Platform:");
                htmlWriter->Element(HtmlElement::TableData, SystemInfo::PlatformAsString(systemInfo.GetPlatform()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "CPU Type:");
                htmlWriter->Element(HtmlElement::TableData, SystemInfo::CpuTypeAsString(systemInfo.GetCpuType()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "CPU Cores:");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(systemInfo.GetNumCpuCores()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Page Size:");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(systemInfo.GetPageSize()));
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);
            
        // if not compiled with NEBULA_DEBUG, display a message
        #if (NEBULA_DEBUG == 0)
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();
        htmlWriter->Text("RefCounted stats not available because application was not compiled with debug information!");
        #else
        htmlWriter->Element(HtmlElement::Heading3, "RefCounted Instances");
        Dictionary<String, RefCounted::Stats> refCountedStats = RefCounted::GetOverallStats();
        /// count overall number of instances
        IndexT i;
        SizeT overallNumInstances = 0;
        for (i = 0; i < refCountedStats.Size(); i++)
        {
            overallNumInstances += refCountedStats.ValueAtIndex(i).numObjects;
        }
        htmlWriter->Text("Number Of Registered Classes: " + String::FromInt(refCountedStats.Size()));
        htmlWriter->LineBreak();
        htmlWriter->Text("Overall Number Of Instances: " + String::FromInt(overallNumInstances));
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "ClassName");
                htmlWriter->Element(HtmlElement::TableHeader, "ClassFourCC");
                htmlWriter->Element(HtmlElement::TableHeader, "InstanceSize");
                htmlWriter->Element(HtmlElement::TableHeader, "NumInstances");
                htmlWriter->Element(HtmlElement::TableHeader, "NumReferences");
            htmlWriter->End(HtmlElement::TableRow);

            for (i = 0; i < refCountedStats.Size(); i++)
            {
                const RefCounted::Stats& curStats = refCountedStats.ValueAtIndex(i);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, curStats.className);
                    if (curStats.classFourCC.IsValid())
                    {
                        htmlWriter->Element(HtmlElement::TableData, curStats.classFourCC.AsString());
                    }
                    else
                    {
                        // legacy Mangalore classes have no FourCC class code
                        htmlWriter->Element(HtmlElement::TableData, "<NULL>");
                    }
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(curStats.instanceSize));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(curStats.numObjects));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(curStats.overallRefCount));
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