//------------------------------------------------------------------------------
//  physicsfeature/physicsfeatureunit.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "physicsfeature/physicsfeatureunit.h"
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
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    world->RegisterType<PhysicsActor>({ .decay = true });
    world->RegisterType<IsKinematic>();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    Game::TimeSourceCreateInfo timeSourceInfo;
    timeSourceInfo.hash = TIMESOURCE_PHYSICS;
    Game::TimeManager::CreateTimeSource(timeSourceInfo);
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
    n_assert(this->physicsWorlds.Contains(world));
    IndexT scene = this->physicsWorlds[world];
    Physics::FlushSimulation(scene);
    FeatureUnit::OnBeforeCleanup(world);
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsFeatureUnit::OnBeginFrame()
{
#if USE_SYNC_UPDATE > 0
    Game::TimeSource* const time = Game::TimeManager::GetTimeSource(TIMESOURCE_PHYSICS);
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
    Game::TimeSource* const time = Game::TimeManager::GetTimeSource(TIMESOURCE_PHYSICS);
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
    Physics::RenderDebug();
}

} // namespace Game
