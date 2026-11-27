//------------------------------------------------------------------------------
//  @file physicseditor.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physicsfeature/editor/physicseditor.h"

#if WITH_NEBULA_EDITOR
#include "basegamefeature/managers/transformhierarchy.h"
#include "game/world.h"
#include "io/ioserver.h"
#include "io/path.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/model/scene.h"
#include "physicsfeature/components/physicsfeature.h"
#include "physicsfeature/managers/physicsmanager.h"

namespace PhysicsFeature
{

// The collider components, as the manager knows them. This is the set the constraint is built from,
// and it is the feature's own list rather than something the editor has a type for.
//
// The ids are read at attach rather than put in a static array, because a component id only exists
// once the component has been registered, and a namespace scope array runs before anything is
// registered.
static Util::Array<Game::ComponentId>
ColliderComponents()
{
    Util::Array<Game::ComponentId> colliders;
    colliders.Append(Game::GetComponentId<PhysicsFeature::BoxCollider>());
    colliders.Append(Game::GetComponentId<PhysicsFeature::SphereCollider>());
    colliders.Append(Game::GetComponentId<PhysicsFeature::CapsuleCollider>());
    colliders.Append(Game::GetComponentId<PhysicsFeature::PlaneCollider>());
    colliders.Append(Game::GetComponentId<PhysicsFeature::MeshCollider>());
    colliders.Append(Game::GetComponentId<PhysicsFeature::HeightFieldCollider>());
    return colliders;
}

//------------------------------------------------------------------------------
/**
    A model asset dropped on the viewport collides as the static mesh it is. The packager writes a
    model's mesh under the model's folder and file, which is the mesh the collider cooks. A model
    without a mesh is left without a body
*/
static void
DropModel(Editor::EditorDroppedAsset& entity)
{
    const IO::Path mesh = IO::Path::File(entity.asset.GetFolder(), entity.asset.GetFile(), "msh");
    if (!IO::Path::ExportExtensions.Contains("msh") || !IO::FileExists(mesh.ExportURI()))
        return;

    PhysicsFeature::StaticBody body;
    entity.Add(Game::GetComponentId<PhysicsFeature::StaticBody>(), &body, sizeof(body));

    PhysicsFeature::MeshCollider collider;
    collider.resource = mesh.AsString();
    collider.primGroup = 0;
    entity.Add(Game::GetComponentId<PhysicsFeature::MeshCollider>(), &collider, sizeof(collider));
}

//------------------------------------------------------------------------------
/**
    The feature registers what it has, and the editor reads the registry at open. A feature that
    registers nothing is never asked for, so this does not check whether an editor is running.
*/
void
RegisterEditorContributions()
{
    Editor::EditorFeatureRegistry::RegisterAddComponentConstraint(ColliderComponents(), 1);
    Editor::EditorFeatureRegistry::RegisterAssetDrop("mdl", Util::Delegate<void(Editor::EditorDroppedAsset&)>::FromFunction<&DropModel>());

    Editor::EditorFeatureRegistry::RegisterOnComponentChangeListener(
        Util::Delegate<void(Editor::Scene*, Game::Entity, Game::ComponentId)>::
            FromFunction<&PhysicsFeature::OnEditorComponentWritten>());
}

//------------------------------------------------------------------------------
/**
    A component value is only applied by a feature when someone tells it, and the editor is the one who
    made the change. A feature that reads a transform chain has to see the subtree as well, and that
    walk is the feature's own.
*/
void
OnEditorComponentWritten(Editor::Scene* scene, Game::Entity gameEntity, Game::ComponentId component)
{
    Game::World* simWorld = scene->GetSimWorld();

    PhysicsFeature::PhysicsManager::MarkComponentChange(simWorld, gameEntity, component);

    const Game::ComponentId localTransform = Game::GetComponentId<Game::HTransform>();
    if (!Editor::Scene::IsTransformComponent(component) && component != localTransform)
        return;

    Game::TransformHierarchy* hierarchy = simWorld->GetHierarchy();
    uint32_t const instance = hierarchy->GetInstance(gameEntity);
    if (instance == Game::TransformHierarchy::InvalidInstance)
        return;

    Util::Array<uint32_t> stack;
    stack.Append(hierarchy->GetFirstChild(instance));
    while (!stack.IsEmpty())
    {
        uint32_t const child = stack.PopBack();
        if (child == Game::TransformHierarchy::InvalidInstance)
            continue;

        // update physics pose if entities transforms change
        PhysicsFeature::PhysicsManager::MarkPoseChanged(simWorld, hierarchy->GetOwner(child));

        uint32_t grandchild = hierarchy->GetFirstChild(child);
        while (grandchild != Game::TransformHierarchy::InvalidInstance)
        {
            stack.Append(grandchild);
            grandchild = hierarchy->GetNextSibling(grandchild);
        }
    }
}

} // namespace PhysicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
