#pragma once
//------------------------------------------------------------------------------
/**
    The physics feature's editor contribution.

    @see Editor::EditorFeatureRegistry

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "game/componentid.h"
#include "game/entity.h"

namespace Editor
{
class Scene;
}

namespace PhysicsFeature
{

/// register the collider number and the component change listener
void RegisterEditorContributions();

/// the editor wrote a component value into the sim world
void OnEditorComponentWritten(Editor::Scene* scene, Game::Entity entity, Game::ComponentId component);

} // namespace PhysicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
