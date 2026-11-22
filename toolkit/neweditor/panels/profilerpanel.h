#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ProfilerPanel

    The frame profiler. It draws the scopes each thread records as boxes on a timeline of one frame,
    and the counters as rows.

    What the panel holds is the view: pause, the held worst frame, the zoom, the region a double click
    marks, and the frame time history. What it draws comes from the profiling contexts and the counters.

    Pause is a keybind against the panel's pause command, which is what makes it rebindable.

    The GPU queues are not drawn here, because the marks on them come from the render submission.
    The graphics feature draws those in its own panel.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "imgui.h"
#include "neweditor/base/timeline.h"
#include "neweditor/core/editorpanel.h"
#include "profiling/profiling.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class ProfilerPanel final : public EditorPanel
{
    __DeclareClass(ProfilerPanel)
public:
    ProfilerPanel();
    virtual ~ProfilerPanel();

    /// draw the frame
    void Draw() override;

protected:
    bool OnSetup() override;

private:
    /// read what the engine records this frame
    void ReadFrame();

    /// read the contexts the engine has
    void ReadProfiling();

    /// add one frame to the history and to the running average
    void AppendFrameTime(float frameTime);

    /// the box for one scope and the boxes under it. Returns the deepest level it drew
    IndexT DrawScope(const Profiling::ProfilingScope& scope, TimelineCanvas& canvas, ImVec2 pos, IndexT level);

    /// the timeline tab
    void DrawTimeline();

    /// the memory tab
    void DrawMemory();

    /// commands
    void CommandPause(const Util::Array<Util::String>& args);

    bool pauseProfiling;
    bool captureWorstFrame;
    bool profileFixedFps;
    bool hideEmptyThreads;

    float averageFrameTime;
    float prevAverageFrameTime;
    float currentFrameTime;
    float worstFrameTime;
    int fixedFps;

    /// the canvas the boxes are drawn on, and the part of the frame the zoom shows
    TimelineCanvas canvas;

    /// the frame times, newest at the end
    static const IndexT MaxHistory = 120;
    Util::Array<float> frametimeHistory;

    /// the threads, sorted by priority
    Util::Array<Profiling::ProfilingContext> contexts;
};

} // namespace Editor
