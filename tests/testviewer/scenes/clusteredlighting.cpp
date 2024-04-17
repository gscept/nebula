#include "scenes.h"
#include "graphics/graphicsserver.h"
#include "models/modelcontext.h"
#include "lighting/lightcontext.h"
#include "visibility/visibilitycontext.h"
#include "decals/decalcontext.h"
#include "imgui.h"
#include "dynui/im3d/im3d.h"
#include "fog/volumetricfogcontext.h"
#include "particles/particlecontext.h"
#include "characters/charactercontext.h"

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

struct Entity
{
    Graphics::GraphicsEntityId entity;
    Models::ModelContext::MaterialInstanceContext* materialInstanceContext;
};
Util::Array<Entity> entities;
//Util::Array<Graphics::GraphicsEntityId> entities;
Util::Array<Util::String> entityNames;
Util::Array<Models::ModelContext::MaterialInstanceContext*> materialContexts;
Util::Array<Graphics::GraphicsEntityId> pointLights;
Util::Array<Graphics::GraphicsEntityId> spotLights;
Util::Array<Graphics::GraphicsEntityId> areaLights;
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
            entities.Append({ id, nullptr });
            int index = (j + NumPointLights) + (i + NumPointLights) * NumPointLights * 2;
            entityNames.Append(Util::String::Sprintf("PointLight%d", index));
            const float red = Math::rand();
            const float green = Math::rand();
            const float blue = Math::rand();
            Lighting::LightContext::RegisterEntity(id);
            Lighting::LightContext::SetupPointLight(id, Math::vec3(red, green, blue), 2500.0f, 10.0f, false);
            Lighting::LightContext::SetPosition(id, Math::point(i * 4, 5, j * 4));
            pointLights.Append(id);
        }
    }

    static const int NumSpotLights = 1;
    for (int i = -NumSpotLights; i < NumSpotLights; i++)
    {
        for (int j = -NumSpotLights; j < NumSpotLights; j++)
        {
            auto id = Graphics::CreateEntity();
            entities.Append({ id, nullptr });
            int index = (j + NumSpotLights) + (i + NumSpotLights) * NumSpotLights * 2;
            entityNames.Append(Util::String::Sprintf("SpotLight%d", index));
            const float red = Math::rand();
            const float green = Math::rand();
            const float blue = Math::rand();

            Lighting::LightContext::RegisterEntity(id);
            Lighting::LightContext::SetupSpotLight(id, Math::vec3(red, green, blue), 2500.0f, 45.0_rad, 60.0_rad, 50.0f, true);
            Lighting::LightContext::SetRotation(id, Math::quatyawpitchroll(0, 0, 0));
            Lighting::LightContext::SetPosition(id, Math::point(i * 4, 2.5, j * 4));
            spotLights.Append(id);
        }
    }

    static const int NumRectLights = 1;
    for (int i = -NumRectLights; i < NumRectLights; i++)
    {
        for (int j = -NumRectLights; j < NumRectLights; j++)
        {
            auto id = Graphics::CreateEntity();
            entities.Append({ id, nullptr });
            int index = (i + NumRectLights) + (j + NumRectLights) * NumRectLights * 2;
            entityNames.Append(Util::String::Sprintf("AreaLight%d", index));
            const float red = Math::rand();
            const float green = Math::rand();
            const float blue = Math::rand();

            Lighting::LightContext::AreaLightShape shapes[] = {
                Lighting::LightContext::AreaLightShape::Rectangle,
                Lighting::LightContext::AreaLightShape::Disk,
                Lighting::LightContext::AreaLightShape::Tube
            };

            auto shape = shapes[index % 3];

            Lighting::LightContext::RegisterEntity(id);
            Lighting::LightContext::SetupAreaLight(id, shape, Math::vec3(red, green, blue), 250.0f, 15, true);
            Lighting::LightContext::SetPosition(id, Math::point(i * 4, 2.5, j * 4));
            Lighting::LightContext::SetRotation(id, Math::quatyawpitchroll(0, 0, 0));
            Lighting::LightContext::SetScale(id, Math::vector(shape == Lighting::LightContext::AreaLightShape::Disk ? 3.0f : 2.0f, 3.0f, 1.0f));
            areaLights.Append(id);
        }
    }

    static const int NumDecals = 1;
    for (int i = -NumDecals; i < NumDecals; i++)
    {
        for (int j = -NumDecals; j < NumDecals; j++)
        {
            auto id = Graphics::CreateEntity();
            entities.Append({ id, nullptr });
            int index = (j + NumDecals) + (i + NumDecals) * NumDecals * 2;
            entityNames.Append(Util::String::Sprintf("Decal%d", index));
            Math::mat4 transform = Math::scaling(Math::vec3(10, 10, 50));
            transform = Math::rotationyawpitchroll(0, Math::deg2rad(90), Math::deg2rad(Math::rand() * 90.0f)) * transform;
            transform.position = Math::vec4(i * 16, 0, j * 16, 1);

            // load textures
            Resources::ResourceId albedo = Resources::CreateResource("systex:white.dds", "decal"_atm, nullptr, nullptr, true);
            Resources::ResourceId normal = Resources::CreateResource("systex:nobump.dds", "decal"_atm, nullptr, nullptr, true);
            Resources::ResourceId material = Resources::CreateResource("systex:default_material.dds", "decal"_atm, nullptr, nullptr, true);

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
            entities.Append({ id, nullptr });
            int index = (j + NumFogVolumes) + (i + NumFogVolumes) * NumFogVolumes * 2;
            entityNames.Append(Util::String::Sprintf("Fog%d", index));
            Math::mat4 transform = Math::scaling(10);
            transform = Math::rotationyawpitchroll(Math::deg2rad(Math::rand() * 90.0f), 0, 0) * transform;
            transform.position = Math::vec4(4 - i * 4, 0, 4 - j * 4, 1);

            const float red = Math::rand();
            const float green = Math::rand();
            const float blue = Math::rand();

            // setup box volume
            Fog::VolumetricFogContext::RegisterEntity(id);
            Fog::VolumetricFogContext::SetupSphereVolume(id, Math::vec3(4 - i * 4, 0, 4 - j * 4), 4.0f, 1.0f, Math::vec3(red, green, blue));
            //Fog::VolumetricFogContext::SetupBoxVolume(id, transform, Math::rand() * 100.0f, Math::vec3(red, green, blue));

            fogVolumes.Append(id);
        }
    }

    tower = Graphics::CreateEntity();
    Graphics::RegisterEntity<ModelContext, ObservableContext>(tower);
    ModelContext::Setup(tower, "mdl:test/test_2.n3", "Viewer", []()
        {
            ObservableContext::Setup(tower, VisibilityEntityType::Model);
        });
    ModelContext::SetTransform(tower, Math::translation(4, 0, -7));
    entities.Append({ tower, nullptr });
    entityNames.Append("Tower");

    ground = Graphics::CreateEntity();
    Graphics::RegisterEntity<ModelContext, ObservableContext>(ground);
    ModelContext::Setup(ground, "mdl:environment/plcholder_world.n3", "Viewer", []()
        {
            ObservableContext::Setup(ground, VisibilityEntityType::Model);
        });
    ModelContext::SetTransform(ground, Math::translation(0,0,0) * Math::scaling(4));
    entities.Append({ ground, nullptr });
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
    entities.Append({ particle, nullptr });
    entityNames.Append("Particle");
    */

    // setup visibility

    const Util::StringAtom modelRes[] = { "mdl:Units/Unit_Archer.n3",  "mdl:Units/Unit_Footman.n3",  "mdl:Units/Unit_Spearman.n3", "mdl:Units/Unit_Rifleman.n3" };
    const Util::StringAtom skeletonRes[] = { "ske:Units/Unit_Archer.nsk",  "ske:Units/Unit_Footman.nsk",  "ske:Units/Unit_Spearman.nsk", "ske:Units/Unit_Rifleman.nsk" };
    const Util::StringAtom animationRes[] = { "ani:Units/Unit_Archer.nax",  "ani:Units/Unit_Footman.nax",  "ani:Units/Unit_Spearman.nax", "ani:Units/Unit_Rifleman.nax" };

    ModelContext::BeginBulkRegister();
    ObservableContext::BeginBulkRegister();
    static const int NumModels = 20;
    int modelIndex = 0;
    materialContexts.Resize((NumModels * 2) * (NumModels * 2));
    CoreGraphics::BatchGroup::Code code = CoreGraphics::BatchGroup::FromName("FlatGeometryLit");

    for (IndexT i = -NumModels; i < NumModels; i++)
    {
        for (IndexT j = -NumModels; j < NumModels; j++)
        {
            Graphics::GraphicsEntityId ent = Graphics::CreateEntity();
            Graphics::RegisterEntity<ModelContext>(ent);
            IndexT entityIndex = entities.Size();
            entities.Append({ ent, nullptr });
            Util::String sid;
            sid.Format("%s: %d", GraphicsEntityToName(ent), ent);
            entityNames.Append(sid);

            const float timeOffset = Math::rand();// (((i + NumModels)* NumModels + (j + NumModels)) % 4) / 3.0f;

            // create model and move it to the front
            ModelContext::Setup(ent, modelRes[modelIndex], "NotA", [ent, entityIndex, modelIndex, i, j, skeletonRes, animationRes, code]()
                {
                    ModelContext::SetTransform(ent, Math::translation(i * 16, 0, j * 16));

                    uint materialIndex = i + (j + NumModels) * (NumModels * 2);
                    entities[entityIndex].materialInstanceContext = &ModelContext::SetupMaterialInstanceContext(ent, code);

                    Graphics::RegisterEntity<Characters::CharacterContext, ObservableContext>(ent);
                    ObservableContext::Setup(ent, VisibilityEntityType::Model);
                    Characters::CharacterContext::Setup(ent, skeletonRes[modelIndex], 0, animationRes[modelIndex], 0, "Viewer");
                    Characters::CharacterContext::PlayClip(ent, nullptr, 0, 0, Characters::Append, 1.0f, 1, Math::rand() * 100.0f, 0.0f, 0.0f, Math::rand() * 100.0f);
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
        spotLightTransform = Math::rotationyawpitchroll(Graphics::GraphicsServer::Instance()->GetTime() * 2 + i, Math::deg2rad(-55), 0);
        spotLightTransform.position = Lighting::LightContext::GetTransform(spotLights[i]).position;
        //Lighting::LightContext::SetTransform(spotLights[i], spotLightTransform);
    }

    for (i = 0; i < decals.Size(); i++)
    {
        Math::mat4 decalTransform = Math::scaling(Math::vec3(10, 10, 50));
        decalTransform = Math::rotationyawpitchroll(Graphics::GraphicsServer::Instance()->GetTime() * 0.1f + i, Math::deg2rad(90), 0) * decalTransform;
        decalTransform.position = Decals::DecalContext::GetTransform(decals[i]).position;
        //Decals::DecalContext::SetTransform(decals[i], decalTransform);
    }

    for (i = 0; i < fogVolumes.Size(); i++)
    {
        Fog::VolumetricFogContext::SetTurbidity(fogVolumes[i], (1.0f + Math::sin(Graphics::GraphicsServer::Instance()->GetTime() * 0.1f + i) * 0.5f) * 100);
    }

    // Allocate instance constants for entities
    IndexT j = 0;
    for (auto entity : entities)
    {
        if (entity.materialInstanceContext != nullptr)
        {
            /*
            CoreGraphics::ConstantBufferOffset offset = ModelContext::AllocateInstanceConstants(entity.entity, entity.materialInstanceContext->batch);
            Unit::UnitInstanceBlock perInstanceBlock;
            if (j % 2 == 0)
                Math::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }.store(perInstanceBlock.TeamColor);
            else
                Math::vec4{ 0.0f, 0.0f, 1.0f, 1.0f }.store(perInstanceBlock.TeamColor);
            CoreGraphics::SetConstants(offset, perInstanceBlock);
            */
        }
        j++;
    }
    /*
        Math::mat4 globalLightTransform = Lighting::LightContext::GetTransform(globalLight);
        Math::mat4 rotY = Math::rotationy(Math::deg2rad(0.1f));
        Math::mat4 rotX = Math::rotationz(Math::deg2rad(0.05f));
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
    //auto modelPool = Resources::GetStreamPool<Models::StreamModelCache>();
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
        //ModelContext::DeregisterEntity(entities[selected]);
        ObservableContext::DeregisterEntity(entities[selected].entity);
        //Characters::CharacterContext::DeregisterEntity(entities[selected]);
        DestroyEntity(entities[selected].entity);
        entities.EraseIndex(selected);
        entityNames.EraseIndex(selected);
        selected = Math::max(selected-1, 0);
    }
    ImGui::End();
    auto id = entities[selected].entity;
    if (Lighting::LightContext::IsEntityRegistered(id))
    {
        Im3d::Vec3 pos = Lighting::LightContext::GetPosition(id);
        if (Im3d::GizmoTranslation("GizmoEntity", pos))
        {
            Lighting::LightContext::SetPosition(id, pos);
        }
    }
    else if (ModelContext::IsEntityRegistered(id))
    {
        Im3d::Mat4 trans = ModelContext::GetTransform(id);
        if (Im3d::Gizmo("GizmoEntity", trans))
        {
            ModelContext::SetTransform(id, trans);
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
