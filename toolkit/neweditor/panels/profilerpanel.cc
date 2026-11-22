//------------------------------------------------------------------------------
//  @file profilerpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/panels/profilerpanel.h"
#include "neweditor/api/shell.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/input/editorkeybinds.h"
#include "framesync/framesynctimer.h"
#include "math/scalar.h"
#include "math/vec4.h"
#include "memory/memory.h"
#include "util/dictionary.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::ProfilerPanel, 'PRFP', Editor::EditorPanel);

// The color a scope is drawn with is picked by its category
static const ImU32 ScopeColors[] = {
    IM_COL32(255, 200, 200, 255),
    IM_COL32(200, 255, 200, 255),
    IM_COL32(200, 200, 255, 255),
    IM_COL32(200, 200, 255, 255),
    IM_COL32(200, 255, 255, 255),
    IM_COL32(255, 255, 200, 255),
};

//------------------------------------------------------------------------------
/**
*/
ProfilerPanel::ProfilerPanel() :
    pauseProfiling(false),
    captureWorstFrame(false),
    profileFixedFps(false),
    hideEmptyThreads(true),
    averageFrameTime(0.0f),
    prevAverageFrameTime(0.0f),
    currentFrameTime(0.0f),
    worstFrameTime(0.0f),
    fixedFps(60)
{
    this->name = "profiler";
    this->canvas.drawList = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
ProfilerPanel::~ProfilerPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Pause is registered as a keybind against the panel's pause command, which is what makes it
    rebindable.
*/
bool
ProfilerPanel::OnSetup()
{
    EditorCommandSystem& commands = this->core->GetCommands();
    commands.Register("pause", "pause or resume the profiler",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<ProfilerPanel, &ProfilerPanel::CommandPause>(this));

    this->core->GetKeybinds().Register("profiler.pause", Input::Key::F5, Input::Modifier::NoModifier, "pause");

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilerPanel::CommandPause(const Util::Array<Util::String>& args)
{
    this->pauseProfiling = !this->pauseProfiling;
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilerPanel::AppendFrameTime(float frameTime)
{
    this->averageFrameTime += frameTime;
    this->frametimeHistory.Append(1.0f / frameTime);
    if (this->frametimeHistory.Size() > MaxHistory)
        this->frametimeHistory.EraseFront();
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilerPanel::ReadProfiling()
{
    this->contexts = Profiling::ProfilingGetContexts();
}

//------------------------------------------------------------------------------
/**
    What the panel draws is what this reads. A held worst frame keeps the frame it catches, and a
    paused profiler keeps what the last frame leaves.

    The frame time comes from the frame sync timer, because that is what the graphics server reads it
    from. The panel does not know which renderer is running.
*/
void
ProfilerPanel::ReadFrame()
{
    if (!FrameSync::FrameSyncTimer::HasInstance())
        return;

    FrameSync::FrameSyncTimer* timer = FrameSync::FrameSyncTimer::Instance();
    float frameTime = (float) timer->GetFrameTime();

    if (this->captureWorstFrame)
    {
        this->currentFrameTime = frameTime;
        if (this->worstFrameTime < this->currentFrameTime)
        {
            this->AppendFrameTime(this->currentFrameTime);
            this->prevAverageFrameTime = this->currentFrameTime;
            this->worstFrameTime = this->currentFrameTime;
            this->ReadProfiling();
        }
        else
        {
            this->currentFrameTime = this->worstFrameTime;
        }
    }
    else if (!this->pauseProfiling)
    {
        this->currentFrameTime = frameTime;
        this->AppendFrameTime(this->currentFrameTime);

        // The average is reported every 35th frame
        if (timer->GetFrameIndex() % 35 == 0)
        {
            this->prevAverageFrameTime = this->averageFrameTime / 35.0f;
            this->averageFrameTime = 0.0f;
        }

        this->ReadProfiling();
    }

    this->contexts.SortWithFunc([](const Profiling::ProfilingContext& a, const Profiling::ProfilingContext& b)
    {
        if (a.priority == b.priority)
            return a.threadName < b.threadName;
        return a.priority > b.priority;
    });
}

//------------------------------------------------------------------------------
/**
    The box for one scope, and the boxes for its children under it. A scope outside the zoom is not
    drawn, so it answers with the level it was called with.
*/
IndexT
ProfilerPanel::DrawScope(const Profiling::ProfilingScope& scope, TimelineCanvas& canvas, ImVec2 pos, IndexT level)
{
    ImU32 color = ScopeColors[scope.category.HashCode() % (sizeof(ScopeColors) / sizeof(ImU32))];

    Util::String label = Util::Format("%s (%4.4f ms)", scope.name, scope.duration * 1000);

    ImVec2 boxMin, boxMax;
    if (!DrawTimelineBox(canvas, pos, scope.start, scope.duration, color, label.AsCharPtr(), boxMin, boxMax))
        return level;

    // The tooltip and the line only come when nothing is panning
    if (ImGui::IsMouseHoveringRect(boxMin, boxMax) && !ImGui::GetIO().KeyShift)
    {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            canvas.view.Highlight(scope.start, scope.duration);

        ImGui::BeginTooltip();
        ImGui::Text("%s [%s] (%4.4f ms) in %s (%d)", scope.name, scope.category.Value(), scope.duration * 1000,
            scope.file, scope.line);
        ImGui::EndTooltip();

        canvas.drawList->AddLine(ImVec2(boxMax.x, canvas.origin.y), ImVec2(boxMax.x, canvas.limit.y),
            IM_COL32(255, 0, 0, 255), 1.0f);
    }

    pos.y += TimelineRowReserve;
    IndexT deepest = level + 1;
    for (IndexT i = 0; i < scope.children.Size(); i++)
        deepest = Math::max(deepest, this->DrawScope(scope.children[i], canvas, pos, level + 1));

    return deepest;
}

//------------------------------------------------------------------------------
/**
    The timeline. The zoom keeps the time under the mouse where it was, so the wheel moves the edges
    rather than the whole range, and a shift drag moves the whole range.
*/
void
ProfilerPanel::DrawTimeline()
{
    ImGui::Checkbox("Hide empty threads", &this->hideEmptyThreads);

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

    for (const Profiling::ProfilingContext& ctx : this->contexts)
    {
        if (this->hideEmptyThreads && ctx.topLevelScopes.Size() == 0)
            continue;

        if (ImGui::CollapsingHeader(ctx.threadName.Value(),
            ctx.topLevelScopes.Size() > 0 ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_OpenOnArrow))
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            this->canvas.width = ImGui::GetContentRegionAvail().x;

            IndexT levels = 0;
            for (IndexT i = 0; i < ctx.topLevelScopes.Size(); i++)
                levels = Math::max(levels, this->DrawScope(ctx.topLevelScopes[i], this->canvas, pos, 0));

            ImGui::SetCursorScreenPos(pos);
            ImGui::Dummy(ImVec2(this->canvas.width, Math::max(1.0f, (float) levels * TimelineRowReserve)));
        }
    }

    this->canvas.view.DrawOverlay(this->canvas);
}

//------------------------------------------------------------------------------
/**
    The counters, and the budgets the counters are measured against.
*/
void
ProfilerPanel::DrawMemory()
{
    const Util::Dictionary<const char*, uint64_t>& counters = Profiling::ProfilingGetCounters();
    for (IndexT i = 0; i < counters.Size(); i++)
    {
        const char* name = counters.KeyAtIndex(i);
        uint64_t val = counters.ValueAtIndex(i);
        if (val >= 1_GB)
            ImGui::LabelText(name, "%.2f GB allocated", val / double(1_GB));
        else if (val >= 1_MB)
            ImGui::LabelText(name, "%.2f MB allocated", val / double(1_MB));
        else if (val >= 1_KB)
            ImGui::LabelText(name, "%.2f KB allocated", val / double(1_KB));
        else
            ImGui::LabelText(name, "%llu B allocated", (unsigned long long) val);
    }

    const Util::Dictionary<const char*, Util::Pair<uint64_t, uint64_t>>& budgets = Profiling::ProfilingGetBudgetCounters();
    for (IndexT i = 0; i < budgets.Size(); i++)
    {
        const char* name = budgets.KeyAtIndex(i);
        const Util::Pair<uint64_t, uint64_t>& val = budgets.ValueAtIndex(i);

        if (val.first >= 1_GB)
            ImGui::LabelText(name, "%.2f GB allocated, %.2f GB left", val.second / double(1_GB),
                (val.first - val.second) / double(1_GB));
        else if (val.first >= 1_MB)
            ImGui::LabelText(name, "%.2f MB allocated, %.2f MB left", val.second / double(1_MB),
                (val.first - val.second) / double(1_MB));
        else if (val.first >= 1_KB)
            ImGui::LabelText(name, "%.2f KB allocated, %.2f KB left", val.second / double(1_KB),
                (val.first - val.second) / double(1_KB));
        else
            ImGui::LabelText(name, "%llu B allocated, %llu B left", (unsigned long long) val.second,
                (unsigned long long) (val.first - val.second));

        float weight = (float)(val.second / double(val.first));

        Math::vec4 green(0, 1, 0, 1);
        Math::vec4 red(1, 0, 0, 1);
        Math::vec4 color = Math::lerp(green, red, Math::sqrt(weight));

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(color.x, color.y, color.z, color.w));
        ImGui::ProgressBar(weight);
        ImGui::PopStyleColor();
    }

#if __WIN32__
    static const char* heapNames[Memory::NumHeapTypes] = {
        "Default Heap",
        "Object Heap",
        "Object Array Heap",
        "Resource Heap",
        "Scratch Heap",
        "String Data Heap",
        "Stream Data Heap",
        "Physics Heap",
        "App Heap",
        "Network Heap",
        "Scripting Heap"
    };
    for (uint i = 0; i < Memory::NumHeapTypes; i++)
    {
        size_t heapUse = Memory::HeapTypeAllocSize[i];
        if (heapUse >= 1_GB)
            ImGui::LabelText(heapNames[i], "%.2f GB allocated", heapUse / double(1_GB));
        else if (heapUse >= 1_MB)
            ImGui::LabelText(heapNames[i], "%.2f MB allocated", heapUse / double(1_MB));
        else if (heapUse >= 1_KB)
            ImGui::LabelText(heapNames[i], "%.2f KB allocated", heapUse / double(1_KB));
        else
            ImGui::LabelText(heapNames[i], "%llu B allocated", (unsigned long long) heapUse);
    }
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
ProfilerPanel::Draw()
{
    // The key is read from the registry, because a bind can be rebound
    const EditorKeyBind* bind = this->core->GetKeybinds().Find("profiler.pause");
    if (bind != nullptr)
        ImGui::Checkbox(Util::Format("Pause (%s)", EditorKeybinds::ComboString(bind->key, bind->mods).AsCharPtr()).AsCharPtr(),
            &this->pauseProfiling);
    else
        ImGui::Checkbox("Pause", &this->pauseProfiling);

    ImGui::Checkbox("Capture worst frame", &this->captureWorstFrame);

    this->ReadFrame();

    if (!this->frametimeHistory.IsEmpty())
    {
        ImGui::Text("ms - %.2f\nFPS - %.2f", this->prevAverageFrameTime * 1000, 1.0f / this->prevAverageFrameTime);
        ImGui::PlotHistogram("####FrameTimes", &this->frametimeHistory[0], this->frametimeHistory.Size(), 0, "Frame",
            FLT_MIN, FLT_MAX, {ImGui::GetContentRegionAvail().x, 90});
        ImGui::Separator();

        ImGui::Checkbox("Fixed FPS", &this->profileFixedFps);
        if (this->profileFixedFps)
        {
            ImGui::InputInt("FPS", &this->fixedFps);
            if (this->fixedFps > 0)
                this->currentFrameTime = 1.0f / float(this->fixedFps);
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    if (ImGui::BeginTabBar("Profiler###Views"))
    {
        if (ImGui::BeginTabItem("Timeline"))
        {
            this->DrawTimeline();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory"))
        {
            this->DrawMemory();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopStyleVar();
}

} // namespace Editor
