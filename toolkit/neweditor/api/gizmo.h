#pragma once
//------------------------------------------------------------------------------
/**
    @namespace
    Editor::Gizmo This namespace is what a viewport tool asks a renderer for. It draws
    shapes on the canvas, and it gives the box around what the renderer draws for an entity.

    The editor does not know the renderer. This struct holds delegates, and the render side fills
    them. The editor gives a shape, a color, a size, and how to draw it. The renderer decides the
    rest. The renderer gives back the box around an entity, and picking and the outlines use that
    box. A delegate that is not filled does nothing.

    Text is the one thing the editor draws itself, because a label is screen space.

    @see Editor::DrawSurface
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "game/world.h"
#include "math/bbox.h"
#include "math/mat4.h"
#include "math/vec4.h"
#include "math/vec3.h"
#include "neweditor/model/scene.h"
#include "neweditor/viewtools/toolview.h"
#include "util/blob.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Editor
{
namespace Gizmo
{

// How to put a shape on the canvas. These are the editor's words, not the renderer's. A renderer
// puts them onto its own queue.
enum DrawFlag
{
    DrawFlag_DefaultDraw = 0,        // as the renderer draws the shape by default
    DrawFlag_Wireframe = 1,          // a box as its edges, not filled
    DrawFlag_AlwaysOnTop = 2,        // over what the frame has already drawn
    DrawFlag_Translucent = 4,        // the color's alpha blends with what is behind it. The shape does
                                     // not write depth
};

// The shapes, always in the same order: the shape, the color, the size, and the flags. The
// size is a line
// width or a point size in pixels. Pixels are the only thing about the screen the editor says.
typedef Util::Delegate<void(const Math::vec3&, const Math::vec3&, const Math::vec4&, float, int)> LineDelegate;
typedef Util::Delegate<void(const Math::bbox&, const Math::vec4&, float, int)> BoxDelegate;
typedef Util::Delegate<void(const Math::vec3&, const Math::vec4&, float, int)> PointDelegate;

// A box at a transform, which is the unit cube the transform puts. A box that is turned is not axis
// aligned, so a gizmo that turns its own box uses this one.
typedef Util::Delegate<void(const Math::mat4&, const Math::vec4&, float, int)> BoxTransformDelegate;

// A ring in a plane: the center, the radius, the plane's normal. How many segments the ring is cut
// into is the renderer's, because that is the side that knows how a ring looks on its canvas.
typedef Util::Delegate<void(const Math::vec3&, float, const Math::vec3&, const Math::vec4&, float, int)> CircleDelegate;

// The box around what a renderer draws for an entity. The answer is the local box, without the
// entity's transform.
// The editor has the transform, so it puts the box where the entity stands. The box comes from a
// component the
// editor does not know, so the renderer answers it.
typedef Util::Delegate<bool(Game::World*, Game::Entity, Math::bbox&)> BoundsDelegate;

//------------------------------------------------------------------------------
/**
    The struct the render side fills. Fill it once, when the feature attaches. Take it back when the
    feature stops, because a delegate holds the object it was bound to.
*/
struct Interface
{
    LineDelegate line;
    BoxDelegate box;
    BoxTransformDelegate boxTransform;
    PointDelegate point;
    CircleDelegate circle;
    BoundsDelegate bounds;
};

/// Fill the struct with what a renderer can do. A shape that is not filled is not drawn.
void Fill(const Interface& interface);
/// Take the struct back.
void Clear();

/// Draw a line between two points in the world.
void DrawLine(const Math::vec3& from, const Math::vec3& to, const Math::vec4& color, float width,
    DrawFlag flags = DrawFlag_DefaultDraw);
/// Draw a box at its own position and size.
void DrawBox(const Math::bbox& box, const Math::vec4& color, float width, DrawFlag flags = DrawFlag_DefaultDraw);
/// Draw the unit cube at a transform. The transform's scale is the box's size.
void DrawBox(const Math::mat4& transform, const Math::vec4& color, float width, DrawFlag flags = DrawFlag_DefaultDraw);
/// Draw a point in the world.
void DrawPoint(const Math::vec3& position, const Math::vec4& color, float size, DrawFlag flags = DrawFlag_DefaultDraw);
/// Draw a ring in a plane. The normal is the plane the ring lies in.
void DrawCircle(const Math::vec3& center, float radius, const Math::vec3& normal, const Math::vec4& color,
    float width, DrawFlag flags = DrawFlag_DefaultDraw);
/// Draw a label at a world point. Text is the one shape the editor draws itself, because it is screen
/// space: the point is put on the canvas with the camera the canvas is drawn with, and the label goes
/// on the gizmo's draw list, which is drawn after the shapes. The draw flags do not apply to it.
void DrawText(const char* text, const Math::vec3& point, const Math::vec4& color);
/// The box around what the renderer draws for an entity, as the local box. Return false when
/// nothing is filled, or when the renderer draws nothing for the entity.
bool GetBounds(Game::World* world, Game::Entity entity, Math::bbox& out);

//------------------------------------------------------------------------------
/**
    What a component gizmo is given for one entity. The gizmo cannot read the scene, the selection,
    or the canvas, so this is what the pass hands it. It is only valid while the pass is running.
*/
struct GizmoFrame
{
    Scene* scene;
    Game::Entity entity;
    /// the component the gizmo was registered for. A handle writes back through it
    Game::ComponentId component;
    /// the name the status line and the 'gizmo list' command use for this gizmo
    Util::String name;
    /// the entity's world transform, as the hierarchy resolves it in the world the canvas draws
    Math::mat4 transform;
    /// the component's value. During a drag this is the value before the drag, because a change is only
    /// previewed into the sim world. Null when the component cannot be read
    const void* value;
    /// the canvas and the camera, for putting a handle on the screen
    const ToolView* view;
    /// true when the entity is the selection, which is when its handles can be grabbed
    bool selected;
    /// the snap increment per axis, zero on an axis that does not snap
    Math::vec3 snap;
};

//------------------------------------------------------------------------------
/**
    One component's gizmo: a function that draws it for one entity. The editor calls it once per
    frame for each selected entity that has the component.

    Registering a gizmo:

    @code{.cpp}
    Editor::Gizmo::ComponentGizmo gizmo;
    gizmo.component = Game::GetComponentId<MyComponent>();
    gizmo.name = "my component";
    gizmo.draw = Util::Delegate<void(const Editor::Gizmo::GizmoFrame&)>::FromFunction<&DrawMyGizmo>();
    Editor::Gizmo::RegisterComponentGizmo(gizmo);
    @endcode

    Drawing a gizmo:

    @code{.cpp}
    void DrawMyGizmo(const Editor::Gizmo::GizmoFrame& frame)
    {
        const MyComponent* base = (const MyComponent*)frame.value;
        if (base == nullptr)
            return;

        MyComponent value = *base;
        Editor::Gizmo::HandleStyle style = Editor::Gizmo::DefaultHandleStyle();
        style.tip = "radius";

        float radius;
        const Math::vec3 at(center + axis * base->radius);
        if (Editor::Gizmo::HandleFloat("radius", base->radius, &radius, at, axis, style))
            value.radius = radius;

        Editor::Gizmo::DrawCircle(center, value.radius, axis, color, 1.0f, Editor::Gizmo::DrawFlag_AlwaysOnTop);

        if (value.radius != base->radius)
            Editor::Gizmo::CommitComponent(frame.scene, frame.entity, frame.component, &value, sizeof(value));
    }
    @endcode
*/
struct ComponentGizmo
{
    Game::ComponentId component;
    Util::String name;
    Util::Delegate<void(const GizmoFrame&)> draw;
};

/// Register a component gizmo
void RegisterComponentGizmo(ComponentGizmo gizmo);
/// Deregister a component gizmo
void UnregisterComponentGizmo(Game::ComponentId component);
/// Deregister all gizmos
void ClearComponentGizmos();
/// Returns what is registered, in the order it was registered
const Util::Array<ComponentGizmo>& GetRegisteredGizmos();

//------------------------------------------------------------------------------
/**
*/
struct HandleStyle
{
    Math::vec4 color;
    
    // the pixels the handle is drawn at, and it is also the distance the cursor has to be within for the handle to take the press.
    float size; 
    
    int flags;
    /// shown on hover
    const char* tip;
};

/// The default handle style: the selection outline's color, a handle big enough to grab.
HandleStyle DefaultHandleStyle();

/// A handle that drives one number. The handle is drawn at handlePos, and the drag moves it along
/// direction axis.
/// Returns true when the user dragged it, and write the new number through out.
/// @param out the new number, measured from the number the handle had when the press happened
bool HandleFloat(const char* id, float value, float* out, const Math::vec3& handlePos,
    const Math::vec3& direction, const HandleStyle& style);

/// A handle that drives a point. A zero direction means the handle moves free, on the plane facing
/// the camera.
/// Returns true when the user is dragging it, and writes the new point through out
/// @param out the new point, measured from the point the handle had when the press happened
bool HandleVec3(const char* id, const Math::vec3& value, Math::vec3* out, const Math::vec3& handlePos,
    const Math::vec3& direction, const HandleStyle& style);

/// The value a handle changed. Written into the sim world while the drag runs, and put through the
/// command manager as one entry when the drag ends.
void CommitComponent(Scene* scene, Game::Entity entity, Game::ComponentId component,
    const void* bytes, SizeT size);

/// Run this frame's pass over the selection. Returns true when this frame belongs to a handle's drag,
/// which includes the frame the drag ends on.
bool DrawComponentGizmos(const ToolView& view);

/// true while a component handle is holding a drag
bool IsDragging();

/// the gizmo the active handle belongs to, or an empty string when nothing is being dragged
const Util::String& ActiveGizmoName();

/// Put the drag back without committing it
void AbortDrag();

//------------------------------------------------------------------------------
// The same functions for the managed side. Vectors cross as three floats, because a native vec3 is
// four floats and the managed side cannot know which of the four it reads.
//
// The draw function is not here: the managed side resolves its own entry point, and the C++ side that
// resolves it registers it through RegisterComponentGizmo.
//
// These are only valid inside the pass.
extern "C"
{

void GizmoLine(const float* from, const float* to, const float* color, float width, int flags);
void GizmoBox(const float* min, const float* max, const float* color, float width, int flags);
void GizmoBoxTransform(const float* transform, const float* color, float width, int flags);
void GizmoPoint(const float* position, const float* color, float size, int flags);
void GizmoCircle(const float* center, float radius, const float* normal, const float* color, float width, int flags);
void GizmoText(const char* text, const float* point, const float* color);

bool GizmoHandleFloat(const char* id, float value, float* out, const float* handlePos, const float* direction,
    const float* color, float size, int flags, const char* tip);
bool GizmoHandleVec3(const char* id, const float* value, float* out, const float* handlePos, const float* direction,
    const float* color, float size, int flags, const char* tip);

// What the managed side cannot read of the frame it is drawing for.
unsigned long long GizmoEntityId();
unsigned int GizmoComponentId();
bool GizmoIsSelected();
void GizmoTransform(float* out16);
bool GizmoProject(const float* world, float* outWindow);
float GizmoSnapAxis(int axis);

} // extern "C"

} // namespace Gizmo

} // namespace Editor
//------------------------------------------------------------------------------
