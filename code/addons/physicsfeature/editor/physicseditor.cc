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

//--------------------------------------------------------------------------
/**
*/
void
RigidbodyAxisLockFieldDrawFunc(Game::Entity, Game::ComponentId, void* data, bool* commit)
{
    Util::BitField<16>* bits = reinterpret_cast<Util::BitField<16>*>(data);
    
    bool linearX = bits->IsSet(0);
    bool linearY = bits->IsSet(1);
    bool linearZ = bits->IsSet(2);
    bool angX = bits->IsSet(3);
    bool angY = bits->IsSet(4);
    bool angZ = bits->IsSet(5);

    ImGui::Text("Lock movement ");
    ImGui::PushID(8833447);
    *(commit) = ImGui::Checkbox("X", &linearX);
    ImGui::SameLine();
    *(commit) |= ImGui::Checkbox("Y", &linearY);
    ImGui::SameLine();
    *(commit) |= ImGui::Checkbox("Z", &linearZ);
    ImGui::PopID();

    ImGui::Text("Lock rotation ");
    ImGui::PushID(8833448);
    *(commit) |= ImGui::Checkbox("X", &angX);
    ImGui::SameLine();
    *(commit) |= ImGui::Checkbox("Y", &angY);
    ImGui::SameLine();
    *(commit) |= ImGui::Checkbox("Z", &angZ);
    ImGui::PopID();

    bits->Clear();
    bits->SetBitIf(0, linearX);
    bits->SetBitIf(1, linearY);
    bits->SetBitIf(2, linearZ);
    bits->SetBitIf(3, angX);
    bits->SetBitIf(4, angY);
    bits->SetBitIf(5, angZ);
}

//--------------------------------------------------------------------------
/**
*/
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

    
    // Override bitfield inspector for RB axis constraints
    Game::ComponentInspection::OverrideField(Game::GetComponentId<Rigidbody>(), "axisLock"_atm, RigidbodyAxisLockFieldDrawFunc);
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
