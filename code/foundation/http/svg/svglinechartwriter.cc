//------------------------------------------------------------------------------
//  svglinechartwriter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "http/svg/svglinechartwriter.h"

namespace Http
{
__ImplementClass(Http::SvgLineChartWriter, 'SLCW', Http::SvgPageWriter);

using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
SvgLineChartWriter::SvgLineChartWriter() :
    xAxisMinVal(0),
    xAxisMaxVal(100),
    yAxisMinVal(0.0f),
    yAxisMaxVal(1.0f)
{
    // empty
}    

//------------------------------------------------------------------------------
/**
*/
SvgLineChartWriter::~SvgLineChartWriter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
SvgLineChartWriter::Open()
{
    // always set virtual dimensions to 1000x250
    this->SetUnitDimensions(1200, 250);
    return SvgPageWriter::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
SvgLineChartWriter::SetupXAxis(const String& axisName, const String& unitName, int minVal, int maxVal)
{
    n_assert(maxVal > minVal);
    this->xAxisName = axisName;
    this->xAxisUnitName = unitName;
    this->xAxisMinVal = minVal;
    this->xAxisMaxVal = maxVal;
}

//------------------------------------------------------------------------------
/**
*/
void
SvgLineChartWriter::SetupYAxis(const String& axisName, const String& unitName, float minVal, float maxVal)
{
    n_assert(maxVal > minVal);
    this->yAxisName = axisName;
    this->yAxisUnitName = unitName;
    this->yAxisMinVal = minVal;
    this->yAxisMaxVal = maxVal;
}

//------------------------------------------------------------------------------
/**
*/
void
SvgLineChartWriter::AddTrack(const String& name, const String& color, const Array<float>& values)
{
    n_assert(values.Size() > 0);
    Track newTrack;
    this->tracks.Append(newTrack);
    this->tracks.Back().name = name;
    this->tracks.Back().color = color;
    this->tracks.Back().values = values;
}

//------------------------------------------------------------------------------
/**
*/
void 
SvgLineChartWriter::AddTrack(const Util::String& name, const Util::String& color, const Util::Array<float>& values, const Util::Array<Timing::Tick>& timeStamps)
{    
    n_assert(values.Size() > 0);
    n_assert(values.Size() == timeStamps.Size());
    Track newTrack;
    this->tracks.Append(newTrack);
    this->tracks.Back().name = name;
    this->tracks.Back().color = color;
    this->tracks.Back().values = values;
    this->tracks.Back().times = timeStamps;

}
//------------------------------------------------------------------------------
/**
*/
void
SvgLineChartWriter::Draw()
{
    // establish a transform for the main diagram
    this->BeginTransformGroup(vec2(10.0f, 10.0f), 0.0f, vec2(1.0f, 1.0f));
    
    // draw light-weight helper lines
    this->BeginPaintGroup("grey", "grey", 1);
    this->Line(0, 0,   1000, 0);
    this->Line(0, 50,  1000, 50);
    this->Line(0, 100, 1000, 100);
    this->Line(0, 150, 1000, 150);
    this->EndGroup();

    // draw the main diagram axis
    this->BeginPaintGroup("black", "black", 2);
    this->Line(0, 0, 0, 200);           // left y-axis
    this->Line(1000, 0, 1000, 200);     // right y-axis
    this->Line(0, 200, 1000, 200);      // x-axis
    this->EndGroup();

    // for each track...
    IndexT trackIndex;
    for (trackIndex = 0; trackIndex < this->tracks.Size(); trackIndex++)
    {
        const Track& curTrack = this->tracks[trackIndex];
        float x = 0.0f;
        float dx = 1.0f / (this->xAxisMaxVal - this->xAxisMinVal);
        float minVal = 1000000.0f;
        float maxVal = -1000000.0f;
        float avgVal = 0.0f;
        float curVal = 0.0f;
        IndexT valIndex = 0;
        SizeT batchSize = 100;
        Array<vec2> points(batchSize + 1, 0);
        Array<Util::String> timeText(batchSize + 1, 0);
        this->BeginPaintGroup("none", curTrack.color, 2);
        while (valIndex < curTrack.values.Size())
        {
            // need to render points in chunks of 100 due to a limitation
            // in TinyXML(?), XML attribute values are limited to about 4096 characters
            float curValue;
            float curY = 0.0f;
            IndexT i;
            for (i = 0; (i < batchSize) && (valIndex < curTrack.values.Size()); i++)
            {
                curValue = curTrack.values[valIndex++];
                curY = 1.0f - (curValue - this->yAxisMinVal) / (this->yAxisMaxVal - this->yAxisMinVal);
                points.Append(vec2(1000.0f * x, 200.0f * curY));
                x += dx;
                minVal = n_min(minVal, curValue);
                maxVal = n_max(maxVal, curValue);
                avgVal += curValue;  
                if (curTrack.times.Size() > 0)
                {           
                    timeText.Append(Util::String::FromFloat((float)Timing::TicksToSeconds(curTrack.times[i])));
                }
            }
            this->PolyLine(points);
            
            if (curTrack.times.Size() > 0)
            {       
                this->EndGroup();
                this->BeginTextGroup(8, "black"); 
                IndexT txtIdx;
                for (txtIdx = 0; txtIdx < timeText.Size(); ++txtIdx)
                {
                    this->Text(points[txtIdx].x, 210, timeText[txtIdx]);        
                }
                this->EndGroup();
                this->BeginPaintGroup("none", curTrack.color, 2);
                timeText.Clear();
            }

            // prepare next batch
            points.Clear();
            points.Append(vec2(1000.f * (x - dx), 200.0f * curY));
        }
        this->EndGroup();
        // compute average value
        avgVal /= curTrack.values.Size();
        curVal = curTrack.values.Back();

        // draw min/max/avg/current lines
        float minY = 1.0f - (minVal - this->yAxisMinVal) / (this->yAxisMaxVal - this->yAxisMinVal);
        float maxY = 1.0f - (maxVal - this->yAxisMinVal) / (this->yAxisMaxVal - this->yAxisMinVal);
        float avgY = 1.0f - (avgVal - this->yAxisMinVal) / (this->yAxisMaxVal - this->yAxisMinVal);
        float curY = 1.0f - (curVal - this->yAxisMinVal) / (this->yAxisMaxVal - this->yAxisMinVal);
        this->BeginPaintGroup("none", curTrack.color, 1);
        this->Line(0.0f, 200.0f * minY, 1000.0f, 200.0f * minY);
        this->Line(0.0f, 200.0f * maxY, 1000.0f, 200.0f * maxY);
        this->Line(0.0f, 200.0f * avgY, 1000.0f, 200.0f * avgY);
        this->Line(0.0f, 200.0f * curY, 1000.0f, 200.0f * curY);
        this->EndGroup();

        // draw min/max/avg text
        String minText;
        String maxText;
        String avgText;
        String curText;
        maxText.Format("max = %.3f", maxVal);
        if (curVal != maxVal) curText.Format("current = %.3f", curVal);
        if (avgVal != maxVal) avgText.Format("avg = %.3f", avgVal);
        if (maxVal != minVal) minText.Format("min = %.3f", minVal);                              

        this->BeginTextGroup(10, curTrack.color);                                                        
        this->Text(1010.0f, 200.0f * maxY, maxText);
        if (curText.IsValid()) this->Text(1010.0f, 200.0f * curY, curText);
        float curOffset = 0;
        const float minDist = 0.04f;
        if (avgY - maxY < minDist)
        {
            avgY += minDist;
            curOffset = minDist;
        }
        if (avgText.IsValid()) this->Text(1010.0f, 200.0f * avgY, avgText);
        if (minY + curOffset - avgY < minDist) minY += curOffset + minDist;
        if (minText.IsValid()) this->Text(1010.0f, 200.0f * minY, minText);        
        this->EndGroup();
    }
    this->EndGroup();
}

} // namespace Http
