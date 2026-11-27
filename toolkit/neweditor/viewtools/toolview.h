#pragma once
//------------------------------------------------------------------------------
/**
    @struct
    Editor::ToolView This struct is what a tool can see of one frame in a viewport: the
    canvas, the camera the canvas is drawn with, and the mouse. The panel fills it in, so a tool
    does not read the keyboard or the mouse itself. The canvas and the mouse are in the same space,
    window coordinates, and CanvasRay and Project go between that space and the world.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "math/line.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec2.h"
#include "math/vec3.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
    The basis a tool works in.
*/
enum FrameOfReference
{
    WorldFrame = 0,
    LocalFrame = 1,
    NumFrames,
};

struct ToolView
{
    /// the canvas: its top left and its size, in window coordinates
    Math::vec2 canvasMin = Math::vec2(0, 0);
    Math::vec2 canvasSize = Math::vec2(0, 0);
    /// the camera this frame's canvas is drawn with, as the two matrices a projection needs. The
    /// render side owns the camera object, and these are what a tool works against.
    Math::mat4 view = Math::mat4();
    Math::mat4 projection = Math::mat4();
    /// the basis the gizmo is drawn and dragged in
    int frame = WorldFrame;

    /// the cursor in window coordinates
    Math::vec2 mouse = Math::vec2(0, 0);
    /// the left button went down over the canvas this frame
    bool pressed = false;
    /// the left button is down, wherever the cursor is
    bool held = false;
    /// the left button came up this frame, wherever the cursor was
    bool released = false;
    /// the right button went down over the canvas this frame. A tool reads this as a request to
    /// cancel what it is holding, without committing it.
    bool rightPressed = false;
    /// the cursor is over the canvas this frame
    bool hovered = false;
    /// the control key is down, which makes a pick add to the selection
    bool ctrl = false;
    /// the left alt key is down. A selection drag moves on the y axis instead of across the plane
    /// when it is held.
    bool alt = false;
    /// a shift key is down
    bool shift = false;
    /// escape went down this frame
    bool escape = false;

    /// What the transform handle did this frame. The handle pass runs before the tools and fills
    /// this in. gizmoUsed is true when a handle has this frame's drag, which is the transform handle
    /// or a component gizmo handle, and a tool that is not the one holding it reads nothing from the
    /// frame. The fields after it say what the drag did since it took hold: the move, the turn, and
    /// the stretch along the handle's own axes.
    bool gizmoUsed = false;
    Math::vec3 gizmoOffset = Math::vec3(0);
    Math::quat gizmoRotation = Math::quat(0, 0, 0, 1);
    Math::vec3 gizmoScaleRatio = Math::vec3(1);

    /// true when a ray can be cast through the canvas
    bool IsValid() const;
    /// A ray in world space through a point given in window coordinates. The line is long, so a
    /// plane intersect falls inside it rather than past its end.
    bool CanvasRay(const Math::vec2& windowPos, Math::line& ray) const;
    /// A world position in window coordinates. Return false when the point is behind the camera.
    bool Project(const Math::vec3& worldPos, Math::vec2& windowPos) const;
};

} // namespace Editor
