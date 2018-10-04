//------------------------------------------------------------------------------
//  stringatompagehandler.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/debug/stringatompagehandler.h"
#include "http/html/htmlpagewriter.h"
#include "util/globalstringatomtable.h"
#include "math/scalar.h"

namespace Debug
{
__ImplementClass(Debug::StringAtomPageHandler, 'SAPH', Http::HttpRequestHandler);

using namespace IO;
using namespace Http;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
StringAtomPageHandler::StringAtomPageHandler()
{
    this->SetName("StringAtoms");
    this->SetDesc("show debug information about StringAtom system");
    this->SetRootLocation("stringatom");
}

//------------------------------------------------------------------------------
/**
*/
void
StringAtomPageHandler::HandleRequest(const Ptr<HttpRequest>& request) 
{
    n_assert(HttpMethod::Get == request->GetMethod());
    Dictionary<String,String> query = request->GetURI().ParseQuery();

    // configure a HTML page writer
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula StringAtom Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "StringAtom System");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");

        // get the current string atom table content, and overall string size
        GlobalStringAtomTable::DebugInfo debugInfo = GlobalStringAtomTable::Instance()->GetDebugInfo();
        htmlWriter->Element(HtmlElement::Heading3, "String Atom Table Info:");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->TableRow2("Number of strings: ", String::FromSize(debugInfo.strings.Size()));
            htmlWriter->TableRow2("Chunk Size: ", String::FromSize(debugInfo.chunkSize));
            htmlWriter->TableRow2("Num chunks: ", String::FromSize(debugInfo.numChunks));
            htmlWriter->TableRow2("Alloc size: ", String::FromSize(debugInfo.allocSize));
            htmlWriter->TableRow2("Used size: ", String::FromSize(debugInfo.usedSize));
            htmlWriter->TableRow2("Growth enabled: ", debugInfo.growthEnabled ? "yes" : "no");
        htmlWriter->End(HtmlElement::Table);

        // dump string atom table
        htmlWriter->Element(HtmlElement::Heading3, "String Atom Table Dump:");
        const SizeT maxElementsInTable = 100;
        IndexT minIndex = 0;
        IndexT maxIndex = n_min(debugInfo.strings.Size(), maxElementsInTable);
        if (query.Contains("min"))
        {
            minIndex = n_max(query["min"].AsInt(), 0);
        }
        if (query.Contains("max"))
        {
            maxIndex = n_min(query["max"].AsInt(), debugInfo.strings.Size());
        }

        // write prev/next links
        if (minIndex > 0)
        {
            String str;
            str.Format("/stringatom?min=%d&max=%d", minIndex - maxElementsInTable, minIndex);
            htmlWriter->AddAttr("href", str);
            htmlWriter->Element(HtmlElement::Anchor, "<< prev|");
        }
        if (maxIndex < debugInfo.strings.Size())
        {
            String str;
            str.Format("/stringatom?min=%d&max=%d", maxIndex, maxIndex + maxElementsInTable);
            htmlWriter->AddAttr("href", str);
            htmlWriter->Element(HtmlElement::Anchor, "|next >>");
        }
        htmlWriter->LineBreak();

        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "Index");
                htmlWriter->Element(HtmlElement::TableHeader, "StringAtom");
            htmlWriter->End(HtmlElement::TableRow);
            IndexT i;
            for (i = minIndex; i < maxIndex; i++)
            {
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->TableRow2(String::FromInt(i), debugInfo.strings[i]);
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

} // namespace Debug