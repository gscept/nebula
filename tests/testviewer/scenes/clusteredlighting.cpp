#include "stdneb.h"
#include "scenes.h"

using namespace Timing;
using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace ClusteredSceneData
{

Graphics::GraphicsEntityId entity;
Graphics::GraphicsEntityId tower;
Graphics::GraphicsEntityId ground;
Graphics::GraphicsEntityId terrain;
Graphics::GraphicsEntityId particle;
Util::Array<Graphics::GraphicsEntityId> entities;
Util::Array<Util::String> entityNames;
Util::Array<Graphics::GraphicsEntityId> pointLights;
Util::Array<Graphics::GraphicsEntityId> spotLights;
Util::Array<Graphics::GraphicsEntityId> decals;
Util::Array<Graphics::GraphicsEntityId> fogVolumes;
Util::Array<CoreGraphics::TextureId> decalTextures;

//------------------------------------------------------------------------------
/**
*/
static const char*
GraphicsEntityToName(GraphicsEntityId id)
{
    if (ModelContext::IsEntityRegistered(id)) return "Model";
    if (Lighting::LightContext::IsEntityRegistered(id)) return "Light";
    return "Entity";
}

//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    static const int NumPointLights = 1;
    for (int i = -NumPointLights; i < NumPointLights; i++)
    {
        for (int j = -NumPointLights; j < NumPointLights; j++)
        {
            auto id = Graphics::CreateEntity();
            entities.Append(id);
            int index = (j + NumPointLights) + (i + NumPointLights) * NumPointLights * 2;
            entityNames.Append(Util::String::Sprintf("PointLight%d", index));
            const float red = Math::n_rand();
            const float green = Math::n_rand();
            const float blue = Math::n_rand();
            Lighting::LightContext::RegisterEntity(id);
            Lighting::LightContext::SetupPointLight(id, Math::vec3(red, green, blue), 25.0f, Math::translation(i * 4, 5, j * 4), 10.0f, false);
            pointLights.Append(id);
        }
    }

    static const int NumSpotLights = 1;
    for (int i = -NumSpotLights; i < NumSpotLights; i++)
    {
        for (int j = -NumSpotLights; j < NumSpotLights; j++)
        {
            auto id = Graphics::CreateEntity();
            entities.Append(id);
            int index = (j + NumSpotLights) + (i + NumSpotLights) * NumSpotLights * 2;
            entityNames.Append(Util::String::Sprintf("SpotLight%d", index));
            const float red = Math::n_rand();
            const float green = Math::n_rand();
            const float blue = Math::n_rand();

            Math::mat4 spotLightMatrix = Math::rotationyawpitchroll(Math::n_deg2rad(120), Math::n_deg2rad(25), 0);
            spotLightMatrix.position = Math::vec4(i * 4, 2.5, j * 4, 1);

            Lighting::LightContext::RegisterEntity(id);
            Lighting::LightContext::SetupSpotLight(id, Math::vec3(red, green, blue), 250.0f, Math::n_deg2rad(45.0f), Math::n_deg2rad(60.0f), spotLightMatrix, 50.0f, true);
            spotLights.Append(id);
        }
    }

    static const int NumDecals = 1;
    for (int i = -NumDecals; i < NumDecals; i++)
    {
        for (int j = -NumDecals; j < NumDecals; j++)
        {
            auto id = Graphics::CreateEntity();
            entities.Append(id);
            int index = (j + NumDecals) + (i + NumDecals) * NumDecals * 2;
            entityNames.Append(Util::String::Sprintf("Decal%d", index));
            Math::mat4 transform = Math::scaling(Math::vec3(10, 10, 50));
            transform = transform * Math::rotationyawpitchroll(0, Math::n_deg2rad(90), Math::n_deg2rad(Math::n_rand() * 90.0f));
            transform.position = Math::vec4(i * 16, 0, j * 16, 1);

            // load textures
            CoreGraphics::TextureId albedo = Resources::CreateResource("tex:system/white.dds", "decal"_atm, nullptr, nullptr, true);
            CoreGraphics::TextureId normal = Resources::CreateResource("tex:test/normalbox_normal.dds", "decal"_atm, nullptr, nullptr, true);
            CoreGraphics::TextureId material = Resources::CreateResource("tex:system/default_material.dds", "decal"_atm, nullptr, nullptr, true);

            // setup decal
            Decals::DecalContext::RegisterEntity(id);
            Decals::DecalContext::SetupDecalPBR(id, transform, albedo, normal, material);

            decalTextures.Append(albedo);
            decalTextures.Append(normal);
            decalTextures.Append(material);
            decals.Append(id);
        }
    }

    static const int NumFogVolumes = 0;
    for (int i = -NumFogVolumes; i < NumFogVolumes; i++)
    {
        for (int j = -NumFogVolumes; j < NumFogVolumes; j++)
        {
            auto id = Graphics::CreateEntity();
            entities.Append(id);
            int index = (j + NumFogVolumes) + (i + NumFogVolumes) * NumFogVolumes * 2;
            entityNames.Append(Util::String::Sprintf("Fog%d", index));
            Math::mat4 transform = Math::scaling(10);
            transform = transform * Math::rotationyawpitchroll(Math::n_deg2rad(Math::n_rand() * 90.0f), 0, 0);
            transform.position = Math::vec4(4 - i * 4, 0, 4 - j * 4, 1);

            const float red = Math::n_rand();
            const float green = Math::n_rand();
            const float blue = Math::n_rand();

            // setup box volume
            Fog::VolumetricFogContext::RegisterEntity(id);
            Fog::VolumetricFogContext::SetupSphereVolume(id, Math::vec3(4 - i * 4, 0, 4 - j * 4), 4.0f, 1.0f, Math::vec3(red, green, blue));
            //Fog::VolumetricFogContext::SetupBoxVolume(id, transform, Math::n_rand() * 100.0f, Math::vec3(red, green, blue));

            fogVolumes.Append(id);
        }
    }

    tower = Graphics::CreateEntity();
    Graphics::RegisterEntity<ModelContext, ObservableContext>(tower);
    ModelContext::Setup(tower, "mdl:test/test.n3", "Viewer", []()
        {
            ObservableContext::Setup(tower, VisibilityEntityType::Model);
        });
    ModelContext::SetTransform(tower, Math::translation(4, 0, -7));
    entities.Append(tower);
    entityNames.Append("Tower");

    ground = Graphics::CreateEntity();
    Graphics::RegisterEntity<ModelContext, ObservableContext>(ground);
    ModelContext::Setup(ground, "mdl:environment/plcholder_world.n3", "Viewer", []()
        {
            ObservableContext::Setup(ground, VisibilityEntityType::Model);
        });
    ModelContext::SetTransform(ground, Math::scaling(4) * Math::translation(0,0,0));
    entities.Append(ground);
    entityNames.Append("Ground");

    /*
    particle = Graphics::CreateEntity();
	Graphics::RegisterEntity<ModelContext, ObservableContext, Particles::ParticleContext>(particle);
    ModelContext::Setup(particle, "mdl:Particles/Build_dust.n3", "Viewer", []()
        {
            ObservableContext::Setup(particle, VisibilityEntityType::Particle);
            Particles::ParticleContext::Setup(particle);
            Particles::ParticleContext::Play(particle, Particles::ParticleContext::RestartIfPlaying);
        });	
	entities.Append(particle);
	entityNames.Append("Particle");
    */

    // setup visibility

    const Util::StringAtom modelRes[] = { "mdl:Units/Unit_Archer.n3",  "mdl:Units/Unit_Footman.n3",  "mdl:Units/Unit_Spearman.n3", "mdl:Units/Unit_Rifleman.n3" };
    //const Util::StringAtom modelRes[] = { "mdl:system/placeholder.n3",  "mdl:system/placeholder.n3",  "mdl:system/placeholder.n3" };
    const Util::StringAtom skeletonRes[] = { "ske:Units/Unit_Archer.nsk3",  "ske:Units/Unit_Footman.nsk3",  "ske:Units/Unit_Spearman.nsk3", "ske:Units/Unit_Rifleman.nsk3" };
    const Util::StringAtom animationRes[] = { "ani:Units/Unit_Archer.nax3",  "ani:Units/Unit_Footman.nax3",  "ani:Units/Unit_Spearman.nax3", "ani:Units/Unit_Rifleman.nax3" };

    ModelContext::BeginBulkRegister();
    ObservableContext::BeginBulkRegister();
    static const int NumModels = 1;
    int modelIndex = 0;
    for (IndexT i = -NumModels; i < NumModels; i++)
    {
        for (IndexT j = -NumModels; j < NumModels; j++)
        {
            Graphics::GraphicsEntityId ent = Graphics::CreateEntity();
            Graphics::RegisterEntity<ModelContext, ObservableContext>(ent);
            entities.Append(ent);
            Util::String sid;
            sid.Format("%s: %d", GraphicsEntityToName(ent), ent);
            entityNames.Append(sid);

            const float timeOffset = Math::n_rand();// (((i + NumModels)* NumModels + (j + NumModels)) % 4) / 3.0f;

            // create model and move it to the front
            ModelContext::Setup(ent, modelRes[modelIndex], "NotA", [ent, modelIndex, i, j, skeletonRes, animationRes]()
                {
                    ModelContext::SetTransform(ent, Math::translation(i * 16, 0, j * 16));
                    ObservableContext::Setup(ent, VisibilityEntityType::Model);
                    //Characters::CharacterContext::Setup(ent, skeletonRes[modelIndex], animationRes[modelIndex], "Viewer");
                    //Characters::CharacterContext::PlayClip(ent, nullptr, 1, 0, Characters::Append, 1.0f, 1, Math::n_rand() * 100.0f, 0.0f, 0.0f, Math::n_rand() * 100.0f);
                });
            
            modelIndex = (modelIndex + 1) % 4;
        }
    }

    ModelContext::EndBulkRegister();
    ObservableContext::EndBulkRegister();
}

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    n_error("implement me!");
}

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
    // animate the spotlights
    IndexT i;
    for (i = 0; i < spotLights.Size(); i++)
    {
        Math::mat4 spotLightTransform;
        spotLightTransform = Math::rotationyawpitchroll(Graphics::GraphicsServer::Instance()->GetTime() * 2 + i, Math::n_deg2rad(-55), 0);
        spotLightTransform.position = Lighting::LightContext::GetTransform(spotLights[i]).position;
        //Lighting::LightContext::SetTransform(spotLights[i], spotLightTransform);
    }

    for (i = 0; i < decals.Size(); i++)
    {
        Math::mat4 decalTransform = Math::scaling(Math::vec3(10, 10, 50));
        decalTransform = decalTransform * Math::rotationyawpitchroll(Graphics::GraphicsServer::Instance()->GetTime() * 0.1f + i, Math::n_deg2rad(90), 0);
        decalTransform.position = Decals::DecalContext::GetTransform(decals[i]).position;
        //Decals::DecalContext::SetTransform(decals[i], decalTransform);
    }

    for (i = 0; i < fogVolumes.Size(); i++)
    {
        Fog::VolumetricFogContext::SetTurbidity(fogVolumes[i], (1.0f + Math::n_sin(Graphics::GraphicsServer::Instance()->GetTime() * 0.1f + i) * 0.5f) * 100);
    }
    /*
        Math::mat4 globalLightTransform = Lighting::LightContext::GetTransform(globalLight);
        Math::mat4 rotY = Math::rotationy(Math::n_deg2rad(0.1f));
        Math::mat4 rotX = Math::rotationz(Math::n_deg2rad(0.05f));
        globalLightTransform = globalLightTransform * rotX * rotY;
        Lighting::LightContext::SetTransform(globalLight, globalLightTransform);
    */
}

//------------------------------------------------------------------------------
/**
*/
void RenderUI()
{
    //Models::ModelId model = ModelContext::GetModel(this->entity);
    //auto modelPool = Resources::GetStreamPool<Models::StreamModelPool>();
    //auto resource = modelPool->GetName(model);    
    //ImGui::Separator();
    //ImGui::Text("Resource: %s", resource.AsString().AsCharPtr());
    //ImGui::Text("State: %s", stateToString(modelPool->GetState(model)));
    //if (ImGui::Button("Browse"))
    //{
    //    ImGui::OpenPopup("Browse for Model");        
    //    this->Browse();
    //}
    //if (ImGui::BeginPopupModal("Browse for Model"))
    //{
    //    ImGui::BeginChild("##browserheader", ImVec2(0, 300), true);// ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y));
    //    ImGui::Columns(2);
    //    ImGui::Text("Folder");
    //    for (int i = 0; i < this->folders.Size(); i++)
    //    {
    //        if (ImGui::Selectable(this->folders[i].AsCharPtr(), i == this->selectedFolder))
    //        {
    //            this->selectedFolder = i;
    //            this->files = IO::IoServer::Instance()->ListFiles("mdl:" + this->folders[i], "*");
    //        }
    //    }
    //    ImGui::NextColumn();
    //    ImGui::Text("Files");
    //        
    //    for (int i = 0; i < this->files.Size(); i++)
    //    {
    //        if (ImGui::Selectable(this->files[i].AsCharPtr(), i == this->selectedFile))
    //        {
    //            this->selectedFile = i;                    
    //        }
    //    }
    //    ImGui::EndChild();
    //    if (ImGui::Button("OK",ImVec2(120, 40))) 
    //    {
    //        ImGui::CloseCurrentPopup(); 
    //        Util::String file = "mdl:" + this->folders[this->selectedFolder] + "/" + this->files[this->selectedFile];                                   
    //        ModelContext::ChangeModel(this->entity, file, "Viewer");
    //    }
    //    ImGui::SameLine();
    //    if (ImGui::Button("Cancel",ImVec2(120, 40))) { ImGui::CloseCurrentPopup(); }
    //    ImGui::EndPopup();
    //}

    ImGui::Begin("Entities", nullptr, 0);
    ImGui::SetWindowSize(ImVec2(240, 400), ImGuiCond_Once);
    ImGui::BeginChild("##entities", ImVec2(0, 300), true);

    static int selected = 0;
    for (int i = 0; i < entityNames.Size(); i++)
    {
        if (ImGui::Selectable(entityNames[i].AsCharPtr(), i == selected))
        {
            selected = i;
        }
    }
    ImGui::EndChild();
    if (ImGui::Button("Delete"))
    {
        ModelContext::DeregisterEntity(entities[selected]);
        ObservableContext::DeregisterEntity(entities[selected]);
        //Characters::CharacterContext::DeregisterEntity(entities[selected]);
        DestroyEntity(entities[selected]);
        entities.EraseIndex(selected);
        entityNames.EraseIndex(selected);
        selected = Math::n_max(selected-1, 0);
    }
    ImGui::End();
    auto id = entities[selected];
    if (ModelContext::IsEntityRegistered(id))
    {
        Im3d::Mat4 trans = ModelContext::GetTransform(id);
        if (Im3d::Gizmo("GizmoEntity", trans))
        {
            ModelContext::SetTransform(id, trans);
        }
    }
    else if (Lighting::LightContext::IsEntityRegistered(id))
    {
        Im3d::Mat4 trans = Lighting::LightContext::GetTransform(id);
        if (Im3d::Gizmo("GizmoEntity", trans))
        {
            Lighting::LightContext::SetTransform(id, trans);
        }
    }
    else if (Fog::VolumetricFogContext::IsEntityRegistered(id))
    {
        Im3d::Mat4 trans = Fog::VolumetricFogContext::GetTransform(id);
        if (Im3d::Gizmo("GizmoEntity", trans))
        {
            Fog::VolumetricFogContext::SetTransform(id, trans);
        }
    }
    else if (Decals::DecalContext::IsEntityRegistered(id))
    {
        Im3d::Mat4 trans = Decals::DecalContext::GetTransform(id);
        if (Im3d::Gizmo("GizmoEntity", trans))
        {
            Decals::DecalContext::SetTransform(id, trans);
        }
    }
}

} // namespace ClusteredSceneData


Scene ClusteredScene =
{
    "ClusteredScene",
    ClusteredSceneData::OpenScene,
    ClusteredSceneData::CloseScene,
    ClusteredSceneData::StepFrame,
    ClusteredSceneData::RenderUI
};
