#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::GraphicsProfilerPanel

    The GPU half of the profiler. The marks on the queues are what the render submission puts there, so
    this panel belongs to the graphics feature rather than to the editor.

    It draws the same timeline the base profiler draws, through the shared canvas, with its own zoom
    and its own highlighted region.

    @see Editor::ProfilerPanel

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "coregraphics/commandbuffer.h"
#include "coregraphics/config.h"
#include "imgui.h"
#include "math/vec4.h"
#include "neweditor/base/timeline.h"
#include "neweditor/core/editorpanel.h"
#include "util/array.h"

namespace GraphicsFeature
{

class GraphicsProfilerPanel final : public Editor::EditorPanel
{
    __DeclareClass(GraphicsProfilerPanel)
public:
    GraphicsProfilerPanel();
    virtual ~GraphicsProfilerPanel();

    /// draw the frame
    void Draw() override;

private:
    /// read what the submission puts on the queues this frame
    void ReadFrame();

    /// the box for one mark and the boxes under it. Returns the deepest level it drew
    IndexT DrawMarker(const CoreGraphics::FrameProfilingMarker& marker, Editor::TimelineCanvas& canvas,
        ImVec2 pos, IndexT level);

    /// the box for a whole queue, with the marks under it. Returns the deepest level it drew
    IndexT DrawQueue(const char* name, CoreGraphics::QueueType queue,
        const Util::Array<CoreGraphics::FrameProfilingMarker>& markers, Editor::TimelineCanvas& canvas,
        ImVec2 pos);

    bool pauseProfiling;
    bool captureWorstFrame;

    float currentFrameTime;
    float worstFrameTime;

    Editor::TimelineCanvas canvas;

    /// the marks the submission puts on the queues. They only exist when profiling is on
#if NEBULA_ENABLE_PROFILING
    Util::Array<CoreGraphics::FrameProfilingMarker> graphicsMarkers;
    Util::Array<CoreGraphics::FrameProfilingMarker> computeMarkers;
#endif
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
