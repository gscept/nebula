#pragma once
//------------------------------------------------------------------------------
/**
    Browser for resources

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Presentation
{

class Profiler : public BaseWindow
{
    __DeclareClass(Profiler)
public:
    Profiler();
    ~Profiler();

    /// Render
    void Run(SaveMode save) override;

    struct HighLightRegion
    {
        float start = -1.0f;
        float end = -1.0f;
    };

private:
    bool pauseProfiling;
    bool captureWorstFrame;
    bool profileFixedFps;
    float averageFrameTime;
    float prevAverageFrameTime;
    float currentFrameTime;
    float worstFrameTime;
    float timeStart = 0.0f;
    float timeEnd = 1.0f;
    int fixedFps;
    HighLightRegion highlightRegion;
    Util::Array<float> frametimeHistory;
    Util::Array<Profiling::ProfilingContext> ProfilingContexts;
    Util::Array<CoreGraphics::FrameProfilingMarker> frameProfilingMarkers;
};

__RegisterClass(Profiler)

} // namespace Presentation
