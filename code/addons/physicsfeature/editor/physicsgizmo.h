#pragma once
//------------------------------------------------------------------------------
/**
    The physics feature's component gizmos: the handles on the canvas instead of typing numbers into
    the inspector. A feature gives the editor one function per component, and the editor calls it for
    the entities that have the component.

    The shapes go through Editor::Gizmo's interface, which the render side fills. A run that fills
    nothing draws nothing.

    @see Editor::Gizmo

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR

namespace PhysicsFeature
{

/// register the colliders' gizmos
void RegisterColliderGizmos();

/// take them back
void ClearColliderGizmos();

} // namespace PhysicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
