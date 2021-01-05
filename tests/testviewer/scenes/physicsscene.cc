//------------------------------------------------------------------------------
/*
    This is an example of a scene file

    The actual scene object is defined at the bottom of the file. This object
    should be extern in the scenes.h file and added to the list of scenes.

    Functions and data is defined within a namespace, so that we don't pollute
    the global namespace too much.

*/
// (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenes.h"
#include "physics/actorcontext.h"
#include "physics/streamactorpool.h"
#include "visibility/visibilitycontext.h"
#include "models/streammodelpool.h"
#include "models/modelcontext.h"
#include "physics/utils.h"
#include "input/inputserver.h"
#include "graphics/graphicsserver.h"
#include "physics/debugui.h"

using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace PhysicsSceneData
{

// Global variables, within namespace
Graphics::GraphicsEntityId ground;
Graphics::GraphicsEntityId tower;
Graphics::GraphicsEntityId camera;
Ptr<Graphics::GraphicsServer> gfxServer;

struct TestObject
{
    Graphics::GraphicsEntityId model;
    Physics::ActorId actor;
};

Util::Array<TestObject> objects;

Resources::ResourceId groundResource;
Physics::ActorId groundActor;
Resources::ResourceId towerResource;
Physics::ActorId towerActor;

IndexT physicsScene;
float v = 0.0f;

bool PvdFrame = false;

//------------------------------------------------------------------------------
/**
*/
void
Spawn(const Math::mat4& trans, Math::vector linvel, Math::vector angvel)
{

    Graphics::GraphicsEntityId ent = Graphics::CreateEntity();

    ModelContext::RegisterEntity(ent);
    ObservableContext::RegisterEntity(ent);
    ModelContext::Setup(ent, "mdl:test/castle_tower.n3", "NotA", [ent]()
                        {
                            ObservableContext::Setup(ent, VisibilityEntityType::Model);
                        });
                    //ModelContext::Setup(ent, "mdl:system/sphere.n3", "NotA");
    ModelContext::SetTransform(ent, trans);

    Physics::ActorId actor = Physics::CreateActorInstance(towerResource, trans, true);

    auto& pactor = Physics::ActorContext::GetActor(actor);
    pactor.userData = (uint64_t)ent.id;

    Physics::ActorContext::SetLinearVelocity(actor, linvel);
    Physics::ActorContext::SetAngularVelocity(actor, angvel);
    objects.Append(TestObject{ ent, actor });
}

//------------------------------------------------------------------------------
/**
*/
void
Shoot(int count)
{
    static Timing::Time last = 0.0;

    Timing::Time now = GraphicsServer::Instance()->GetTime();
    if (now - last > 0.05)
    {
        last = now;

        Math::mat4 trans = Math::inverse(CameraContext::GetTransform(camera));
        Math::point cameraPos = trans.position;
        while (count--)
        {
            Math::vector offset;// = Math::Randomvec4(5.0f);
            trans.position = cameraPos + offset;
            Spawn(trans, xyz(trans.z_axis) * -50.0f, Math::vector(Math::n_rand(-10.0f, 10.0f)));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateTransform(Physics::ActorId id, Math::mat4 const& trans)
{
    ModelContext::SetTransform(Graphics::GraphicsEntityId{ (Ids::Id32)Physics::ActorContext::GetActor(id).userData }, trans);
}

//------------------------------------------------------------------------------
/**
*/
void
SpawnRandom(int amount)
{
    for (int i = 0; i < amount; i++)
    {
        Math::mat4 trans = Math::translation(Math::vector(Math::n_rand(-200.0f, 200.0f), 150.0f, Math::n_rand(-200.00f, 200.0f)));
        Spawn(trans, Math::vector(0.0), Math::vector(Math::n_rand(-3.0f, 3.0f)));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteRandom(int amount)
{
    amount = Math::n_min(amount, objects.Size());
    for (int i = 0; i < amount; i++)
    {
        IndexT f = Math::n_rand(0, objects.Size() - 1);
        auto& obj = objects[f];
        ObservableContext::DeregisterEntity(obj.model);
        ModelContext::DeregisterEntity(obj.model);
        Graphics::DestroyEntity(obj.model);
        Physics::DestroyActorInstance(obj.actor);
        objects.EraseIndexSwap(f);
    }
}

//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    Physics::Setup();
    physicsScene = Physics::CreateScene();
    IndexT dummyMaterial = Physics::CreateMaterial("dummy"_atm, 0.8, 0.6, 0.3, 1.0);

    ground = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(ground);
    Models::ModelContext::Setup(ground, "mdl:test/groundplane.n3", "ExampleScene", []()
                                {
                                    Visibility::ObservableContext::Setup(ground, Visibility::VisibilityEntityType::Model);
                                });

    groundResource = Resources::CreateResource("phys:test/groundplane.actor", "Viewer", nullptr, nullptr, true);
    groundActor = Physics::CreateActorInstance(groundResource, Math::mat4(), false);


    Models::ModelContext::SetTransform(ground, Math::translation(Math::vec3(0, 0, 0)));

    tower = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(tower);
    Models::ModelContext::Setup(tower, "mdl:test/castle_tower.n3", "ExampleScene", []()
                                {
                                    Visibility::ObservableContext::Setup(tower, Visibility::VisibilityEntityType::Model);
                                });
    Models::ModelContext::SetTransform(tower, Math::translation(Math::vec3(2, 0, 0)));

    towerResource = Resources::CreateResource("phys:test/tower.actor", "Viewer", nullptr, nullptr, true);

    gfxServer = GraphicsServer::Instance();
    camera = gfxServer->GetCurrentView()->GetCamera();

    v = 0.0f;
};

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    Graphics::DestroyEntity(tower);
    Graphics::DestroyEntity(ground);
};

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
    const Timing::Time delta = gfxServer->GetFrameTime();
    Physics::Update(delta);
    Models::ModelContext::SetTransform(tower, Math::translation(Math::vec3(0, 0, Math::n_sin(v))));
    v += 0.01f;
    auto inputServer = Input::InputServer::Instance();
    if (inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::Space))
    {
        if (inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::LeftShift))
        {
            Shoot(20);
        }
        else
        {
            Shoot(1);
        }
    }

    for (auto const& o : objects)
    {
        UpdateTransform(o.actor, Physics::ActorContext::GetTransform(o.actor));
    }
};


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
    ImGui code can be placed here.
*/
void RenderUI()
{
 
    //Util::String count = Util::String::Sprintf("Total OBjects: %d", this->objects.Size());
    ImGui::Begin("OBjects", nullptr, 0);
    ImGui::SetWindowSize(ImVec2(240, 400));
    ImGui::Text("Total Objects %d", objects.Size());
    static int count = 100;
    ImGui::SliderInt("Objects to add/kill", &count, 1, 1000);
    if (ImGui::Button("Spawn!"))
    {
        SpawnRandom(count);
    }
    if (ImGui::Button("Destroy!"))
    {
        DeleteRandom(count);
    }

    ImGui::BeginChild("##objects", ImVec2(0, 300), true);
    static int selected = -1;
    for (int i = 0; i < objects.Size(); i++)
    {
        Util::String sid;
        sid.Format("%s: %d", GraphicsEntityToName(objects[i].model), objects[i].model);
        if (ImGui::Selectable(sid.AsCharPtr(), i == selected))
        {
            selected = i;
        }
    }
    ImGui::EndChild();
    ImGui::End();
};

} // namespace ExampleSceneData

// ---------------------------------------------------------

Scene PhysicsScene =
{
    "PhysicsScene",
    PhysicsSceneData::OpenScene,
    PhysicsSceneData::CloseScene,
    PhysicsSceneData::StepFrame,
    PhysicsSceneData::RenderUI
};