//------------------------------------------------------------------------------
//  @file graphicsfeature/editor/graphicsprofilerpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/graphicsprofilerpanel.h"

#if WITH_NEBULA_EDITOR
#include "coregraphics/graphicsdevice.h"
#include "framesync/framesynctimer.h"
#include "math/scalar.h"
#include "util/string.h"
#include "imgui.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::GraphicsProfilerPanel, 'GVPR', Editor::EditorPanel);

//------------------------------------------------------------------------------
/**
*/
GraphicsProfilerPanel::GraphicsProfilerPanel() :
    pauseProfiling(false),
    captureWorstFrame(false),
    currentFrameTime(0.0f),
    worstFrameTime(0.0f)
{
    this->name = "gpu";
    this->canvas.drawList = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
GraphicsProfilerPanel::~GraphicsProfilerPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    A held worst frame keeps the marks it catches, and a paused panel keeps what the last frame leaves.

    The frame time comes from the frame sync timer, because that is what the graphics server reads it
    from.
*/
void
GraphicsProfilerPanel::ReadFrame()
{
#if NEBULA_ENABLE_PROFILING
    if (!FrameSync::FrameSyncTimer::HasInstance())
        return;

    float frameTime = (float) FrameSync::FrameSyncTimer::Instance()->GetFrameTime();

    if (this->captureWorstFrame)
    {
        this->currentFrameTime = frameTime;
        if (this->worstFrameTime < this->currentFrameTime)
        {
            this->worstFrameTime = this->currentFrameTime;
            this->graphicsMarkers = CoreGraphics::GetProfilingMarkers(CoreGraphics::GraphicsQueueType);
            this->computeMarkers = CoreGraphics::GetProfilingMarkers(CoreGraphics::ComputeQueueType);
        }
        else
        {
            this->currentFrameTime = this->worstFrameTime;
        }
    }
    else if (!this->pauseProfiling)
    {
        this->currentFrameTime = frameTime;
        this->graphicsMarkers = CoreGraphics::GetProfilingMarkers(CoreGraphics::GraphicsQueueType);
        this->computeMarkers = CoreGraphics::GetProfilingMarkers(CoreGraphics::ComputeQueueType);
    }
#endif
}

//------------------------------------------------------------------------------
/**
    A mark is in nanoseconds, so the frame time is read the same way the scopes are.
*/
IndexT
GraphicsProfilerPanel::DrawMarker(const CoreGraphics::FrameProfilingMarker& marker, Editor::TimelineCanvas& canvas,
    ImVec2 pos, IndexT level)
{
    double start = (double) (marker.start + marker.cpuBegin) / 1000000000.0;
    double duration = (double) marker.duration / 1000000000.0;

    ImU32 color = IM_COL32((int) (marker.color.x * 255.0f), (int) (marker.color.y * 255.0f),
        (int) (marker.color.z * 255.0f), (int) (marker.color.w * 255.0f));

    Util::String label = Util::Format("%s (%4.4f ms)", marker.name.AsCharPtr(), duration * 1000);

    ImVec2 boxMin, boxMax;
    if (!Editor::DrawTimelineBox(canvas, pos, start, duration, color, label.AsCharPtr(), boxMin, boxMax))
        return level;

    if (ImGui::IsMouseHoveringRect(boxMin, boxMax) && !ImGui::GetIO().KeyShift)
    {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            canvas.view.Highlight(start, duration);

        ImGui::BeginTooltip();
        ImGui::Text("%s (%4.4f ms)", marker.name.AsCharPtr(), duration * 1000);
        ImGui::EndTooltip();
    }

    pos.y += Editor::TimelineRowReserve;
    IndexT deepest = level + 1;
    for (IndexT i = 0; i < marker.children.Size(); i++)
        deepest = Math::max(deepest, this->DrawMarker(marker.children[i], canvas, pos, level + 1));

    return deepest;
}

//------------------------------------------------------------------------------
/**
    A queue is drawn as one box with the marks under it, because the queue itself has no mark.
*/
IndexT
GraphicsProfilerPanel::DrawQueue(const char* name, CoreGraphics::QueueType queue,
    const Util::Array<CoreGraphics::FrameProfilingMarker>& markers, Editor::TimelineCanvas& canvas, ImVec2 pos)
{
    if (markers.IsEmpty())
        return 0;

    const CoreGraphics::FrameProfilingMarker& first = markers.Front();
    const CoreGraphics::FrameProfilingMarker& last = markers.Back();

    CoreGraphics::FrameProfilingMarker total;
    total.color = NEBULA_MARKER_WHITE;
    total.queue = queue;
    total.name = name;
    total.gpuBegin = first.gpuBegin;
    total.gpuEnd = last.gpuBegin;
    total.start = first.start;
    total.cpuBegin = first.cpuBegin;
    total.duration = (last.start + last.duration + last.cpuBegin) - (first.start + first.cpuBegin);
    total.children = markers;

    return this->DrawMarker(total, canvas, pos, 0);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsProfilerPanel::Draw()
{
#if NEBULA_ENABLE_PROFILING
    ImGui::Checkbox("Pause", &this->pauseProfiling);
    ImGui::Checkbox("Capture worst frame", &this->captureWorstFrame);

    this->ReadFrame();

    this->canvas.drawList = ImGui::GetWindowDrawList();
    this->canvas.origin = ImGui::GetCursorScreenPos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    this->canvas.limit = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
    this->canvas.width = ImGui::GetContentRegionAvail().x;
    this->canvas.rowHeight = ImGui::GetTextLineHeight();
    this->canvas.view.frameTime = this->currentFrameTime;

    ImGui::DragFloatRange2("Time range", &this->canvas.view.startSection, &this->canvas.view.endSection, 0.01f,
        0.0f, 1.0f, "%.3f", "%.3f", ImGuiSliderFlags_AlwaysClamp);

    float range = this->canvas.view.endSection - this->canvas.view.startSection;
    if (range <= 0.0f)
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyShift)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    if (io.MouseWheel != 0.0f && this->canvas.width > 0.0f && ImGui::IsWindowHovered() && io.KeyShift)
    {
        float mouseSection = Math::clamp((ImGui::GetMousePos().x - this->canvas.origin.x) / this->canvas.width,
            0.0f, 1.0f);
        this->canvas.view.Zoom(mouseSection, io.MouseWheel);
    }

    if (io.KeyShift && ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        float delta = -(ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f).x / this->canvas.width) * range;
        this->canvas.view.Pan(delta);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    }

    ImVec2 pos = ImGui::GetCursorScreenPos();
    this->canvas.width = ImGui::GetContentRegionAvail().x;

    IndexT levels = this->DrawQueue("Graphics Queue", CoreGraphics::GraphicsQueueType, this->graphicsMarkers,
        this->canvas, pos);
    pos.y += Math::max(IndexT(1), levels - 1) * Editor::TimelineRowReserve;

    levels = this->DrawQueue("Compute Queue", CoreGraphics::ComputeQueueType, this->computeMarkers, this->canvas, pos);

    ImGui::SetCursorScreenPos(pos);
    ImGui::Dummy(ImVec2(this->canvas.width, Math::max(1.0f, (float) levels * Editor::TimelineRowReserve)));

    this->canvas.view.DrawOverlay(this->canvas);
#else
    ImGui::Text("Profiling is not enabled in this build");
#endif
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
