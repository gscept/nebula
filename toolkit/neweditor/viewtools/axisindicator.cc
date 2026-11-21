//------------------------------------------------------------------------------
//  @file axisindicator.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/viewtools/axisindicator.h"
#include "math/mat4.h"
#include "math/scalar.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "imgui.h"

namespace Editor
{
namespace AxisIndicator
{

// The basis is drawn at a fixed size in the corner, because it says which way the camera is turned
// rather than
// how far away it is. A corner too small for it gets the size it has. The margin is what keeps an
// axis pointing
// straight up, and the letter on its tip, inside the corner
static const float Radius = 40.0f;
static const float Margin = 24.0f;
static const float LineThickness = 3.0f;
static const float TipSize = 3.0f;
static const float LabelGap = 9.0f;

// +X, +Y, +Z
static const Math::vec3 Directions[3] =
{
    Math::vec3(1.0f, 0.0f, 0.0f),
    Math::vec3(0.0f, 1.0f, 0.0f),
    Math::vec3(0.0f, 0.0f, 1.0f)
};
static const ImVec4 Colors[3] =
{
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
    ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
    ImVec4(0.0f, 0.0f, 1.0f, 1.0f)
};
static const char* Labels[3] = { "X", "Y", "Z" };

// An axis as it lands on the screen: its tip, how much of it foreshortening left on the screen, how
// far out
// from the camera it points, and the color it is drawn with. Depth is positive away from the
// viewer, so the
// axes drawn in order of it come out far to near
struct Axis
{
    ImVec2 tip;
    float length;
    float depth;
    ImU32 color;
};

//------------------------------------------------------------------------------
/**
*/
void
Draw(const Math::mat4& view)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList == nullptr)
        return;

    // The corner is the corner of the item the call is made over, which in the viewport is the
    // canvas
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    if (max.x <= min.x || max.y <= min.y)
    {
        // Nothing to draw over, so the corner is the one of the space the window has left for the
        // call
        min = ImGui::GetCursorScreenPos();
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        max = ImVec2(min.x + avail.x, min.y + avail.y);
    }

    const float radius = Math::min(Radius, Math::min(max.x - min.x, max.y - min.y) * 0.5f - Margin);
    if (radius <= 0.0f)
        return;

    const ImVec2 center(max.x - Margin - radius, min.y + Margin + radius);

    Axis axes[3];
    for (int i = 0; i < 3; i++)
    {
        // The view matrix is the world to camera transform, so a direction through it with w 0 is
        // the axis as
        // the camera sees it: x right, y up, z towards the viewer. Screen y goes down, which is why
        // y is
        // negated for the tip, and depth is the negated z
        const Math::vec4 seen = view * Math::vec4(Directions[i], 0.0f);

        const float dx = seen.x * radius;
        const float dy = -seen.y * radius;

        axes[i].tip = ImVec2(center.x + dx, center.y + dy);
        axes[i].length = sqrtf(dx * dx + dy * dy);
        axes[i].depth = -seen.z;
        axes[i].color = ImGui::GetColorU32(Colors[i]);
    }

    // Far to near, so the axis coming towards the viewer is the one on top
    int order[3] = { 0, 1, 2 };
    for (int i = 0; i < 3; i++)
    {
        for (int j = i + 1; j < 3; j++)
        {
            if (axes[order[j]].depth > axes[order[i]].depth)
            {
                int tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }

    for (int i = 0; i < 3; i++)
    {
        const Axis& axis = axes[order[i]];
        drawList->AddLine(center, axis.tip, axis.color, LineThickness);

        // An axis pointing straight at the camera has nothing left on the screen, and its tip is
        // the origin
        if (axis.length > TipSize)
            drawList->AddCircleFilled(axis.tip, TipSize, axis.color);
    }

    // The letters come after the lines, so an axis drawn over another still has its letter readable
    for (int i = 0; i < 3; i++)
    {
        const Axis& axis = axes[order[i]];

        // The letter sits out along the axis, or on the origin when the axis points straight at the
        // camera
        ImVec2 off(0.0f, 0.0f);
        if (axis.length > 1e-4f)
            off = ImVec2((axis.tip.x - center.x) / axis.length * LabelGap, (axis.tip.y - center.y) / axis.length * LabelGap);

        const char* label = Labels[order[i]];
        const ImVec2 size = ImGui::CalcTextSize(label);

        // The letter is kept inside the corner, so an axis pointing straight up does not put it
        // outside the canvas
        ImVec2 pos(axis.tip.x + off.x - size.x * 0.5f, axis.tip.y + off.y - size.y * 0.5f);
        pos.x = Math::max(Math::min(pos.x, max.x - size.x), min.x);
        pos.y = Math::max(Math::min(pos.y, max.y - size.y), min.y);
        drawList->AddText(pos, axis.color, label);
    }

    // The origin, on top of the axes
    drawList->AddCircleFilled(center, LineThickness, ImGui::GetColorU32(ImVec4(0.85f, 0.85f, 0.85f, 1.0f)));
}

} // namespace AxisIndicator
} // namespace Editor
