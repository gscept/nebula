//------------------------------------------------------------------------------
//  svgtestpagehandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/debug/svgtestpagehandler.h"
#include "http/svg/svgpagewriter.h"
#include "http/svg/svglinechartwriter.h"

namespace Debug
{
__ImplementClass(Debug::SvgTestPageHandler, 'SVTP', Http::HttpRequestHandler);

using namespace Http;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
SvgTestPageHandler::SvgTestPageHandler()
{
    this->SetName("SVG Rendering Test");
    this->SetDesc("test SVG rendering functionality");
    this->SetRootLocation("svgtest");
}

//------------------------------------------------------------------------------
/**
*/
bool
SvgTestPageHandler::TestShapeRendering(const Ptr<HttpRequest>& request)
{
    // setup a SVG page writer
    Ptr<SvgPageWriter> svgWriter = SvgPageWriter::Create();
    svgWriter->SetStream(request->GetResponseContentStream());
    svgWriter->SetCanvasDimensions(1000, 1000);
    svgWriter->SetUnitDimensions(1000, 1000);
    if (svgWriter->Open())
    {
        // render an outline
        svgWriter->BeginPaintGroup("none", "black", 1);
            svgWriter->Rect(0, 0, 1000, 1000);
        svgWriter->EndGroup();

        // render rectangles
        svgWriter->BeginPaintGroup("red", "black", 1);
            svgWriter->Rect(100, 100, 150, 150);
            svgWriter->Rect(300, 300, 150, 150);
        svgWriter->EndGroup();
        svgWriter->BeginPaintGroup("black", "red", 1);
            svgWriter->Rect(300, 100, 150, 150);
            svgWriter->Rect(100, 300, 150, 150);
        svgWriter->EndGroup();

        // render circles
        svgWriter->BeginPaintGroup("red", "black", 1);
            svgWriter->Circle(700, 700, 50);
            svgWriter->Circle(800, 800, 50);
        svgWriter->EndGroup();
        svgWriter->BeginPaintGroup("black", "red", 1);
            svgWriter->Circle(700, 800, 50);
            svgWriter->Circle(800, 700, 50);
        svgWriter->EndGroup();

        // render a polyline
        svgWriter->BeginPaintGroup("none", "green", 1);
            Array<float2> points;
            int x;
            for (x = 100; x < 400; x += 2)
            {
                points.Append(float2(float(x), 750.0f + n_rand(-250, 250)));
            }
            svgWriter->PolyLine(points);
        svgWriter->EndGroup();

        // render some text
        svgWriter->BeginTextGroup(30, "black");
            svgWriter->Text(600, 250, "Hello World");
        svgWriter->EndGroup();

        // done...
        svgWriter->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
SvgTestPageHandler::TestLineChartRendering(const Ptr<HttpRequest>& request)
{
    // setup a SVG line chart writer
    Ptr<SvgLineChartWriter> writer = SvgLineChartWriter::Create();
    writer->SetStream(request->GetResponseContentStream());
    writer->SetCanvasDimensions(1024, 256);
    if (writer->Open())
    {
        writer->SetupXAxis("frame", "frames", -512, 0);
        writer->SetupYAxis("fps", "1/sec", 0.0f, 60.0f);
        Array<float> values(512, 0);
        IndexT i;
        for (i = 0; i < 512; i++)
        {
            values.Append(n_rand(10.0f, 45.0f));
        }
        writer->AddTrack("fps", "green", values);
        writer->Draw();
        writer->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
SvgTestPageHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    if (this->TestLineChartRendering(request))
    {
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

} // namespace Debug