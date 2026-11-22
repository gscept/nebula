#pragma once
//------------------------------------------------------------------------------
/**
    The timeline the profiler panels draw: one box per entry, on a row, scaled by the frame time.

    Two panels draw it. One draws the scopes the threads record, the other the marks the render
    submission puts. What they share is the canvas, the zoom, the marked region, and the box.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "imgui.h"

namespace Editor
{

// The height a row reserves in the window. It is more than the box, so rows do not touch
static const float TimelineRowReserve = 20.0f;

//------------------------------------------------------------------------------
/**
    The part of the frame the zoom shows, and the region a double click marks. Both are fractions
    of the frame, and the frame time is what turns them into seconds.
*/
struct TimelineView
{
    float startSection = 0.0f;
    float endSection = 1.0f;
    double frameTime = 0.0;

    /// the region a double click marks. Negative is no region
    float highlightStart = -1.0f;
    float highlightDuration = -1.0f;

    /// true when an entry is inside the zoom
    bool Contains(double start, double duration) const;
    /// zoom around a point, so the point stays where it is
    void Zoom(float mouseSection, float wheel);
    /// move the whole range
    void Pan(float deltaSection);
    /// mark the region a double click lands on
    void Highlight(double start, double duration);
    /// clear the region
    void Clear();
    /// the overlay and the two lines for the marked region. A right click inside it clears it
    void DrawOverlay(struct TimelineCanvas& canvas);
};

//------------------------------------------------------------------------------
/**
    The canvas a set of rows is drawn on. The view is part of it, because a box is placed through the
    zoom.
*/
struct TimelineCanvas
{
    ImDrawList* drawList;
    /// where the timeline starts
    ImVec2 origin;
    /// the bottom right corner of the window, which is where a box is clipped
    ImVec2 limit;
    /// the width the boxes are drawn in
    float width;
    /// the height of one box
    float rowHeight;

    TimelineView view;
};

//------------------------------------------------------------------------------
/**
    One box on the canvas, on the row at pos. Returns false when the entry is outside the zoom, and
    then the row is not used. outMin and outMax are the box, which is what a hover check reads.
*/
bool DrawTimelineBox(const TimelineCanvas& canvas, const ImVec2& pos, double start, double duration,
    ImU32 color, const char* label, ImVec2& outMin, ImVec2& outMax);

} // namespace Editor
