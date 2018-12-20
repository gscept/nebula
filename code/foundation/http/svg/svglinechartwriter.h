#pragma once
//------------------------------------------------------------------------------
/**
    @class Http::SvgLineChartWriter
    
    Specialized SVG page writer which draws a line chart diagrams.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "http/svg/svgpagewriter.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace Http
{
class SvgLineChartWriter : public SvgPageWriter
{
    __DeclareClass(SvgLineChartWriter);
public:
    /// constructor
    SvgLineChartWriter();
    /// destructor
    virtual ~SvgLineChartWriter();
    
    /// begin writing to the stream
    virtual bool Open();
    /// setup the x axis
    void SetupXAxis(const Util::String& axisName, const Util::String& unitName, int minVal, int maxVal);
    /// setup the y axis
    void SetupYAxis(const Util::String& axisName, const Util::String& unitName, float minVal, float maxVal);
    /// add a value track
    void AddTrack(const Util::String& name, const Util::String& color, const Util::Array<float>& values);
    /// add a value track with timestamps
    void AddTrack(const Util::String& name, const Util::String& color, const Util::Array<float>& values, const Util::Array<Timing::Tick>& timeStamps);
    /// draw diagram
    void Draw();

private:
    struct Track
    {
        Util::String name;
        Util::String color;
        Util::Array<float> values;
        Util::Array<Timing::Tick> times;
    };

    Util::String xAxisName;
    Util::String xAxisUnitName;
    int xAxisMinVal;
    int xAxisMaxVal;
    Util::String yAxisName;
    Util::String yAxisUnitName;
    float yAxisMinVal;
    float yAxisMaxVal;
    Util::Array<Track> tracks;
};

} // namespace Http
//------------------------------------------------------------------------------
   