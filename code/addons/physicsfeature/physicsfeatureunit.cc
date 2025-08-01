//------------------------------------------------------------------------------
//  physicsfeature/physicsfeatureunit.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "physics/debugui.h"
#include "physicsinterface.h"
#include "basegamefeature/managers/timemanager.h"
#include "physicsfeature/managers/physicsmanager.h"
#include "physics/actorcontext.h"
#include "game/api.h"
#include "components/physicsfeature.h"

#define USE_SYNC_UPDATE 0

namespace PhysicsFeature
{
__ImplementClass(PhysicsFeature::PhysicsFeatureUnit, 'PXFU' , Game::FeatureUnit);
__ImplementSingleton(PhysicsFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
PhysicsFeatureUnit::PhysicsFeatureUnit() : simulating(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
PhysicsFeatureUnit::~PhysicsFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnAttach()
{
    this->RegisterComponentType<PhysicsActor>({ .decay = true, .OnInit = &PhysicsManager::InitPhysicsActor });
    this->RegisterComponentType<IsKinematic>();
    this->RegisterComponentType<Character>({ .decay = true });
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    this->cl_debug_draw_physics = Core::CVarGet("cl_debug_draw_physics");

    Game::TimeSourceCreateInfo timeSourceInfo;
    timeSourceInfo.hash = TIMESOURCE_PHYSICS;
    Game::Time::CreateTimeSource(timeSourceInfo);
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    this->AttachManager(PhysicsManager::Create());

    Physics::Setup();

    IndexT defaultScene = Physics::CreateScene();
    this->physicsWorlds.Add(world, defaultScene);

    Physics::SetOnSleepCallback([](Physics::ActorId* actors, SizeT num)
    {
        // FIXME: This assumes all actors are in the default world.
        //        This might not be true and we should allow multiple physics stages as well.
        Game::World* world = Game::GetWorld(WORLD_DEFAULT);
        Game::ComponentId staticPid = Game::GetComponentId("Static"_atm);
        for (IndexT i = 0; i < num; i++)
        {
            n_assert(Physics::ActorContext::IsValid(actors[i]));
            Physics::Actor& actor = Physics::ActorContext::GetActor(actors[i]);
            Game::Entity entity = Game::Entity::FromId((Ids::Id32)actor.userData);
            n_assert(world->IsValid(entity) && world->HasInstance(entity));
            world->AddComponent<Game::Static>(entity);
        }
    });
    Physics::SetOnWakeCallback([](Physics::ActorId* actors, SizeT num)
    {
        // FIXME: This assumes all actors are in the default world.
        //        This might not be true and we should allow multiple physics stages as well.
        Game::World* world = Game::GetWorld(WORLD_DEFAULT);
        Game::ComponentId staticPid = Game::GetComponentId<Game::Static>();
        for (IndexT i = 0; i < num; i++)
        {
            n_assert(Physics::ActorContext::IsValid(actors[i]));
            Physics::Actor& actor = Physics::ActorContext::GetActor(actors[i]);
            Game::Entity entity = Game::Entity::FromId((Ids::Id32)actor.userData);
            n_assert(world->IsValid(entity) && world->HasInstance(entity));
            world->RemoveComponent<Game::Static>(entity);
        }
    });
    Physics::SetEventCallback([](const Util::Array<Physics::ContactEvent>& buffer)
    {
        Game::World* world = Game::GetWorld(WORLD_DEFAULT);
        for (auto const& contact : buffer)
        {
            if (Physics::ActorContext::IsValid(contact.actor0))
            {
                Physics::Actor& actor = Physics::ActorContext::GetActor(contact.actor0);
                Game::Entity entity = Game::Entity::FromId((Ids::Id32)actor.userData);
                if (world->IsValid(entity))
                {
                    PhysicsFeature::ContactEventMessage::Send(entity, contact);
                }
            }
            if (Physics::ActorContext::IsValid(contact.actor1))
            {
                Physics::Actor& actor = Physics::ActorContext::GetActor(contact.actor1);
                Game::Entity entity = Game::Entity::FromId((Ids::Id32)actor.userData);
                if (world->IsValid(entity))
                {
                    PhysicsFeature::ContactEventMessage::Send(entity, contact);
                }
            }            
        }
    });

}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnDeactivate()
{   
    FeatureUnit::OnDeactivate();
    for (auto const& scene : this->physicsWorlds)
    {
        Physics::DestroyScene(scene.Value());
    }
    Physics::ShutDown();
}


//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnBeforeCleanup(Game::World* world)
{
    if (this->physicsWorlds.Contains(world))
    {
        IndexT scene = this->physicsWorlds[world];
        Physics::FlushSimulation(scene);
        FeatureUnit::OnBeforeCleanup(world);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsFeatureUnit::OnBeginFrame()
{
#if USE_SYNC_UPDATE > 0
    Game::TimeSource* const time = Game::Time::GetTimeSource(TIMESOURCE_PHYSICS);
    Physics::Update(time->frameTime);
#else
    if (!simulating) return;

    for (auto const& scene : this->physicsWorlds)
    {
        Physics::EndSimulating(scene.Value());
    }
    simulating = false;
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnDecay()
{
    FeatureUnit::OnDecay();
#if USE_SYNC_UPDATE == 0
    Game::TimeSource* const time = Game::Time::GetTimeSource(TIMESOURCE_PHYSICS);
    for (auto const& scene : this->physicsWorlds)
    {
        Physics::BeginSimulating(time->frameTime, scene.Value());
    }
    simulating = true;
#endif
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsFeatureUnit::OnRenderDebug()
{
    int drawPhysics = Core::CVarReadInt(this->cl_debug_draw_physics);
    if (this->debugDrawPhysicsLastValue != drawPhysics)
    {
        Physics::EnableDebugDrawing(drawPhysics);
    }

    this->debugDrawPhysicsLastValue = drawPhysics;

    if (!drawPhysics)
        return;

    Physics::RenderDebug();
}

} // namespace Game
