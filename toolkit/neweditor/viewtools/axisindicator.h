#pragma once
//------------------------------------------------------------------------------
/**
    @namespace
    Editor::AxisIndicator The world basis drawn in the top right corner of the viewport:

    +X red, +Y green, +Z blue.

    Call it over the canvas, after the item the corner belongs to, because the draw list is ordered
    by the calls made in a frame and an item drawn later would cover the lines.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "imgui.h"
#include "math/mat4.h"

namespace Editor
{

namespace AxisIndicator
{

/// Draw the basis in the top right corner of the last item drawn in the window, which in the
/// viewport is the
/// canvas. Without an item to draw over, the corner of the space the window has left for the call
/// is used
void Draw(const Math::mat4& view);

} // namespace AxisIndicator

} // namespace Editor
