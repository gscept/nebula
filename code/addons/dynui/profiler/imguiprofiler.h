#pragma once
//------------------------------------------------------------------------------
/**
    Imgui Profiler UI

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "profiling/profiling.h"
#include "coregraphics/commandbuffer.h"

namespace Dynui
{

class ImguiProfiler : public Core::RefCounted
{
    __DeclareClass(ImguiProfiler);
    __DeclareInterfaceSingleton(ImguiProfiler);

public:

    /// Constructor
    ImguiProfiler();
    /// Destructor
    ~ImguiProfiler();

    /// Capture the frame
    void Capture();

    /// Render, call this per-frame
    void Render(Timing::Time frameTime, IndexT frameIndex);

    /// Pause profiler
    void TogglePause();

private:
    bool pauseProfiling;
    bool showFrameProfiler;
    bool profileFixedFps;
    float averageFrameTime;
    float prevAverageFrameTime;
    float currentFrameTime;
    int fixedFps;
    Util::Array<float> frametimeHistory;
    Util::Array<Profiling::ProfilingContext> profilingContexts;
    Util::Array<CoreGraphics::FrameProfilingMarker> frameProfilingMarkers;

};

} // namespace Dynui
