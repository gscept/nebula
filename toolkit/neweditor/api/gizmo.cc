//------------------------------------------------------------------------------
//  @file gizmo.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/api/gizmo.h"

namespace Editor
{
namespace Gizmo
{

// What a render side filled. The editor calls through this and does not know who filled it
static Interface filled;

//------------------------------------------------------------------------------
/**
*/
void
Fill(const Interface& interface)
{
    filled = interface;
}

//------------------------------------------------------------------------------
/**
    Taken back at the deactivate of the feature that filled it, because a delegate holds the object
    it was bound to.
*/
void
Clear()
{
    filled = Interface();
}

//------------------------------------------------------------------------------
/**
*/
void
DrawLine(const Math::vec3& from, const Math::vec3& to, const Math::vec4& color, float width, DrawFlag flags)
{
    if (!filled.line.IsValid())
        return;

    filled.line(from, to, color, width, flags);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawBox(const Math::bbox& box, const Math::vec4& color, float width, DrawFlag flags)
{
    if (!filled.box.IsValid())
        return;

    filled.box(box, color, width, flags);
}

//------------------------------------------------------------------------------
/**
*/
void
DrawPoint(const Math::vec3& position, const Math::vec4& color, float size, DrawFlag flags)
{
    if (!filled.point.IsValid())
        return;

    filled.point(position, color, size, flags);
}

//------------------------------------------------------------------------------
/**
*/
bool
GetBounds(Game::World* world, Game::Entity entity, Math::bbox& out)
{
    if (!filled.bounds.IsValid())
        return false;

    return filled.bounds(world, entity, out);
}

} // namespace Gizmo

} // namespace Editor
//------------------------------------------------------------------------------
