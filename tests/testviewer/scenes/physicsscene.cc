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
#include "lighting/lightcontext.h"
#include "imgui.h"
#include "characters/charactercontext.h"
#include "visibility/visibilitycontext.h"
#include "models/modelcontext.h"
#include "input/keyboard.h"
#include "input/inputserver.h"
#include "graphics/graphicsserver.h"
#include "physics/debugui.h"
#include "viewerapp.h"
#include "graphics/cameracontext.h"

using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace PhysicsSceneData
{

// Global variables, within namespace
Graphics::GraphicsEntityId ground;
Graphics::GraphicsEntityId tower;
Graphics::GraphicsEntityId camera;
Graphics::GraphicsEntityId knight;

Ptr<Graphics::GraphicsServer> gfxServer;

struct TestObject
{
    Graphics::GraphicsEntityId model;
    Physics::ActorId actor;
};

Util::Array<TestObject> objects;

Physics::ActorResourceId groundResource;
Physics::ActorId groundActor;
Physics::ActorResourceId towerResource;
Physics::ActorId towerActor;
Physics::ActorResourceId knightResource;
Physics::ActorId knightActor;


IndexT physicsScene;
float v = 0.0f;

bool PvdFrame = false;

//------------------------------------------------------------------------------
/**
*/
void
Spawn(const Math::transform& trans, Math::vector linvel, Math::vector angvel)
{

    Graphics::GraphicsEntityId ent = Graphics::CreateEntity();
    ModelContext::RegisterEntity(ent);
    ModelContext::Setup(ent, "mdl:test/castle_tower.n3", "NotA", [=]()
    {
        ObservableContext::RegisterEntity(ent);
        ObservableContext::Setup(ent, VisibilityEntityType::Model);
    });

    Math::mat4 mat = Math::affine(trans.scale, trans.scale, trans.position);
    ModelContext::SetTransform(ent, mat);
    Physics::ActorId actor = Physics::CreateActorInstance(towerResource, trans, Physics::Dynamic, (uint64_t)ent.id);

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

        Math::mat4 mat = Math::inverse(CameraContext::GetView(camera));
        Math::transform trans = Math::transform::FromMat4(mat);
        Math::point cameraPos = trans.position;
        while (count--)
        {
            Math::vector offset;// = Math::Randomvec4(5.0f);
            trans.position = (cameraPos + offset).vec;
            Spawn(trans, xyz(mat.z_axis) * -50.0f, Math::vector(Math::rand(-10.0f, 10.0f)));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateTransform(Physics::Actor const& actor)
{
    ModelContext::SetTransform(Graphics::GraphicsEntityId{ (Ids::Id32)actor.userData }, Physics::ActorContext::GetTransform(actor.id));
}

//------------------------------------------------------------------------------
/**
*/
void
SpawnRandom(int amount)
{
    for (int i = 0; i < amount; i++)
    {
        Math::transform trans(Math::vector(Math::rand(-200.0f, 200.0f), 150.0f, Math::rand(-200.00f, 200.0f)));
        Spawn(trans, Math::vector(0.0), Math::vector(Math::rand(-3.0f, 3.0f)));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteRandom(int amount)
{
    amount = Math::min(amount, objects.Size());
    for (int i = 0; i < amount; i++)
    {
        IndexT f = Math::rand(0, objects.Size() - 1);
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
    physicsScene = Physics::CreateScene();
    Physics::SetActiveActorCallback(UpdateTransform, physicsScene);

    ground = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(ground);
    Models::ModelContext::Setup(ground, "mdl:test/groundplane.n3", "ExampleScene", []()
    {
        Visibility::ObservableContext::Setup(ground, Visibility::VisibilityEntityType::Model);
    });

    knight = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(knight);
    Models::ModelContext::Setup(knight, "mdl:test/Unit_Knight.n3", "ExampleScene", []()
    {
        Visibility::ObservableContext::Setup(knight, Visibility::VisibilityEntityType::Model);
        Characters::CharacterContext::RegisterEntity(knight);
        Characters::CharacterContext::Setup(knight, "ske:test/Unit_Knight.nsk", 0, "ani:test/Unit_Knight.nax", 0, "Viewer");
        Characters::CharacterContext::PlayClip(knight, nullptr, 0, 0, Characters::Append, 1.0f, 1, Math::rand() * 100.0f, 0.0f, 0.0f, Math::rand() * 100.0f);

    });
    knightResource = Resources::CreateResource("phys:test/Unit_Knight.actor", "Viewer", nullptr, nullptr, true);
    knightActor = Physics::CreateActorInstance(knightResource, Math::transform(), Physics::Kinematic, knight.id);


    groundResource = Resources::CreateResource("phys:test/groundplane.actor", "Viewer", nullptr, nullptr, true);
    groundActor = Physics::CreateActorInstance(groundResource, Math::transform(), Physics::Static, ground.id);


    Models::ModelContext::SetTransform(ground, Math::translation(Math::vec3(0, 0, 0)));

    tower = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(tower);
    Models::ModelContext::Setup(tower, "mdl:Buildings/castle_tower.n3", "ExampleScene", []()
    {
        Visibility::ObservableContext::Setup(tower, Visibility::VisibilityEntityType::Model);
    });

    towerResource = Resources::CreateResource("phys:test/castle_tower.actor", "Viewer", nullptr, nullptr, true);

    towerActor = Physics::CreateActorInstance(towerResource, Math::transform(Math::vec3(4, 15, 4)), Physics::ActorType::Dynamic, tower.id);



    gfxServer = Graphics::GraphicsServer::Instance();
    camera = Tests::SimpleViewerApplication::Instance()->GetDefaultCamera();
    v = 0.0f;
};

static void RemoveEntity(Graphics::GraphicsEntityId Id)
{
    Visibility::ObservableContext::DeregisterEntity(Id);
    Models::ModelContext::DeregisterEntity(Id);
    Graphics::DestroyEntity(Id);
}
//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    RemoveEntity(tower);
    RemoveEntity(ground);
    Physics::DestroyActorInstance(groundActor);
    Physics::DestroyActorInstance(towerActor);
    for (auto const& obj : objects)
    {
        RemoveEntity(obj.model);
        Physics::DestroyActorInstance(obj.actor);
    }
    objects.Clear();
    Physics::DestroyScene(physicsScene);
};

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
    const Timing::Time delta = gfxServer->GetFrameTime();
    Physics::Update(delta);
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
    Physics::RenderUI(CameraContext::GetView(camera));
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