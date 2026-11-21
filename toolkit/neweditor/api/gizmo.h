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

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game/entity.h"
#include "game/world.h"
#include "math/bbox.h"
#include "math/vec4.h"
#include "math/vec3.h"
#include "util/delegate.h"

namespace Editor
{

namespace Gizmo
{

// How to put a shape on the canvas. These are the editor's words, not the renderer's. A renderer
// puts them onto
// its own queue.
enum DrawFlag
{
    DrawFlag_DefaultDraw = 0,        // as the renderer draws the shape by default
    DrawFlag_Wireframe = 1,          // a box as its edges, not filled
    DrawFlag_AlwaysOnTop = 2,        // over what the frame has already drawn
};

// The three shapes, always in the same order: the shape, the color, the size, and the flags. The
// size is a line
// width or a point size in pixels. Pixels are the only thing about the screen the editor says.
typedef Util::Delegate<void(const Math::vec3&, const Math::vec3&, const Math::vec4&, float, int)> LineDelegate;
typedef Util::Delegate<void(const Math::bbox&, const Math::vec4&, float, int)> BoxDelegate;
typedef Util::Delegate<void(const Math::vec3&, const Math::vec4&, float, int)> PointDelegate;

// The box around what a renderer draws for an entity. The answer is the local box, without the
// entity's transform.
// The editor has the transform, so it puts the box where the entity stands. The box comes from a
// component the
// editor does not know, so the renderer answers it.
typedef Util::Delegate<bool(Game::World*, Game::Entity, Math::bbox&)> BoundsDelegate;

//------------------------------------------------------------------------------
/**
    The struct the render side fills. Fill it once, when the feature attaches. Take it back when the
    feature stops, because a delegate holds the object it is bound to.
*/
struct Interface
{
    LineDelegate line;
    BoxDelegate box;
    PointDelegate point;
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
/// Draw a point in the world.
void DrawPoint(const Math::vec3& position, const Math::vec4& color, float size, DrawFlag flags = DrawFlag_DefaultDraw);
/// The box around what the renderer draws for an entity, as the local box. Return false when
/// nothing is filled,
/// or when the renderer draws nothing for the entity.
bool GetBounds(Game::World* world, Game::Entity entity, Math::bbox& out);

} // namespace Gizmo

} // namespace Editor
//------------------------------------------------------------------------------
