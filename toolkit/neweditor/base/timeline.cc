//------------------------------------------------------------------------------
//  @file timeline.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/base/timeline.h"
#include "math/scalar.h"

namespace Editor
{

// The text inside a box starts this far from its left edge
static const float TextPad = 5.0f;

//------------------------------------------------------------------------------
/**
*/
bool
TimelineView::Contains(double start, double duration) const
{
    if (this->frameTime <= 0.0)
        return false;

    double frameStart = this->startSection * this->frameTime;
    double frameEnd = this->endSection * this->frameTime;

    return start + duration >= frameStart && start <= frameEnd;
}

//------------------------------------------------------------------------------
/**
    The point under the mouse keeps its place, so the edges move rather than the whole range.
*/
void
TimelineView::Zoom(float mouseSection, float wheel)
{
    float range = this->endSection - this->startSection;
    if (range <= 0.0f)
        return;

    float mouseSectionTime = this->startSection + mouseSection * range;
    float newRange = Math::clamp(range * Math::clamp(1.0f - wheel * 0.1f, 0.1f, 10.0f), 0.01f, 1.0f);

    this->startSection = mouseSectionTime - mouseSection * newRange;
    this->endSection = this->startSection + newRange;

    if (this->startSection < 0.0f)
    {
        this->startSection = 0.0f;
        this->endSection = Math::min(newRange, 1.0f);
    }
    if (this->endSection > 1.0f)
    {
        this->endSection = 1.0f;
        this->startSection = Math::max(0.0f, 1.0f - newRange);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TimelineView::Pan(float deltaSection)
{
    float range = this->endSection - this->startSection;

    this->startSection += deltaSection;
    this->endSection += deltaSection;

    if (this->startSection < 0.0f)
    {
        this->startSection = 0.0f;
        this->endSection = Math::min(this->startSection + range, 1.0f);
    }
    if (this->endSection > 1.0f)
    {
        this->endSection = 1.0f;
        this->startSection = Math::max(0.0f, this->endSection - range);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TimelineView::Highlight(double start, double duration)
{
    if (this->frameTime <= 0.0)
        return;

    this->highlightStart = (float)(start / this->frameTime);
    this->highlightDuration = (float)(duration / this->frameTime);
}

//------------------------------------------------------------------------------
/**
*/
void
TimelineView::Clear()
{
    this->highlightStart = -1.0f;
    this->highlightDuration = -1.0f;
}

//------------------------------------------------------------------------------
/**
    The marked region is drawn at the zoom it is seen through. A right click inside it clears it.
*/
void
TimelineView::DrawOverlay(TimelineCanvas& canvas)
{
    if (this->highlightStart < 0.0f || this->highlightDuration <= 0.0f)
        return;

    float range = this->endSection - this->startSection;
    if (range <= 0.0f || canvas.width <= 0.0f)
        return;

    float regionStart = (this->highlightStart - this->startSection) / range;
    float regionEnd = regionStart + this->highlightDuration / range;
    float x0 = canvas.origin.x + regionStart * canvas.width;
    float x1 = canvas.origin.x + regionEnd * canvas.width;

    canvas.drawList->AddRectFilled(ImVec2(x0, canvas.origin.y), ImVec2(x1, canvas.limit.y), IM_COL32(255, 255, 0, 60));
    canvas.drawList->AddLine(ImVec2(x0, canvas.origin.y), ImVec2(x0, canvas.limit.y), IM_COL32_WHITE, 1.0f);
    canvas.drawList->AddLine(ImVec2(x1, canvas.origin.y), ImVec2(x1, canvas.limit.y), IM_COL32_WHITE, 1.0f);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        float mouseSection = (ImGui::GetMousePos().x - canvas.origin.x) / canvas.width;
        if (mouseSection >= regionStart && mouseSection <= regionEnd)
            this->Clear();
    }
}

//------------------------------------------------------------------------------
/**
    Only the part of the entry the zoom shows is drawn, and the box is clipped to the window.
*/
bool
DrawTimelineBox(const TimelineCanvas& canvas, const ImVec2& pos, double start, double duration,
    ImU32 color, const char* label, ImVec2& outMin, ImVec2& outMax)
{
    const TimelineView& view = canvas.view;
    if (!view.Contains(start, duration))
        return false;

    double frameStart = view.startSection * view.frameTime;
    double frameTime = (view.endSection - view.startSection) * view.frameTime;

    double entryStart = start;
    double entryDuration = duration;
    if (entryStart < frameStart)
    {
        entryDuration -= frameStart - entryStart;
        entryStart = frameStart;
    }

    float startX = pos.x + (float)((entryStart - frameStart) / frameTime) * canvas.width;
    float stopX = startX + Math::max((float)(entryDuration / frameTime) * canvas.width, 1.0f);

    outMin = ImVec2(startX, pos.y);
    outMax = ImVec2(Math::min(stopX, canvas.limit.x), Math::min(pos.y + canvas.rowHeight, canvas.limit.y));

    canvas.drawList->PushClipRect(outMin, outMax, true);
    canvas.drawList->AddRectFilled(outMin, outMax, color);
    canvas.drawList->AddRect(outMin, outMax, IM_COL32(128, 128, 128, 128));
    canvas.drawList->AddText(ImVec2(startX + TextPad, pos.y), IM_COL32_BLACK, label);
    canvas.drawList->PopClipRect();

    return true;
}

} // namespace Editor
