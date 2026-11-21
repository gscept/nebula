//------------------------------------------------------------------------------
//  physics.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/tools/selectiontool.h"
#include "editor/cmds.h"
#include "physics/debugui.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/scene.h"
#include "graphics/cameracontext.h"
#include "game/componentinspection.h"
#include "physicsinterface.h"
#include "physicsfeature/components/physicsfeature.h"

using namespace Editor;

namespace Presentation
{

//------------------------------------------------------------------------------
/**
    The inspector field of a collider material, a choice of the materials in the physics material table
*/
static void
DrawMaterialField(Game::Entity owner, Game::ComponentId component, void* data, bool* commit)
{
    Util::StringAtom& value = *(Util::StringAtom*)data;
    bool known = false;
    for (SizeT i = 0; i < ::Physics::GetNrMaterials(); i++)
    {
        known |= ::Physics::GetMaterial(i).name == value;
    }

    Util::String preview = value.IsValid() ? value.Value() : "";
    if (!known)
    {
        // physics uses the fallback material for a name the table doesn't have
        preview.Append(" (not in the material table)");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
    }
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool open = ImGui::BeginCombo("##material", preview.AsCharPtr());
    if (!known)
    {
        ImGui::PopStyleColor();
    }
    if (open)
    {
        for (SizeT i = 0; i < ::Physics::GetNrMaterials(); i++)
        {
            const ::Physics::Material& material = ::Physics::GetMaterial(i);
            // internal materials start with an underscore
            if (!material.name.IsValid() || material.name.Value()[0] == '_')
            {
                continue;
            }
            ImGui::PushID((int)i);
            const bool selected = material.name == value;
            if (ImGui::Selectable(material.name.Value(), selected))
            {
                value = material.name;
                *commit = true;
            }
            ImGui::SetItemTooltip("Static friction %.2f, dynamic friction %.2f, restitution %.2f, density %.2f",
                material.material->getStaticFriction(), material.material->getDynamicFriction(),
                material.material->getRestitution(), material.density);
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
            ImGui::PopID();
        }
        ImGui::Separator();
        ImGui::TextDisabled("Materials are defined in work/data/tables/physicsmaterials.json");
        ImGui::EndCombo();
    }
}

//------------------------------------------------------------------------------
/**
*/
Physics::Physics()
{
    this->additionalFlags = ImGuiWindowFlags_(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    // collider materials are chosen from the material table instead of picked like assets
    const Game::ComponentId colliders[] = {
        Game::GetComponentId<PhysicsFeature::BoxCollider>(),
        Game::GetComponentId<PhysicsFeature::SphereCollider>(),
        Game::GetComponentId<PhysicsFeature::CapsuleCollider>(),
        Game::GetComponentId<PhysicsFeature::PlaneCollider>(),
        Game::GetComponentId<PhysicsFeature::MeshCollider>(),
        Game::GetComponentId<PhysicsFeature::HeightFieldCollider>(),
    };
    for (const Game::ComponentId& collider : colliders)
    {
        Game::ComponentInspection::OverrideField(collider, "material", &DrawMaterialField);
    }
}

//------------------------------------------------------------------------------
/**
*/
Physics::~Physics()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Physics::Run(SaveMode save)
{
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        Scene* sceneWindow = (Presentation::Scene*)Presentation::SceneWindow;
        this->defaultCamera = ViewGetCamera(sceneWindow->viewPort.GetView());
    }
    if (this->defaultCamera == Graphics::GraphicsEntityId::Invalid())
    {
        return;
    }
    ::Physics::RenderUI(Graphics::CameraContext::GetView(this->defaultCamera));
}

} // namespace Presentation
