//------------------------------------------------------------------------------
//  physicsfeature/physicsfeatureunit.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "physicsinterface.h"
#include "physics/physxstate.h"
#include "graphics/graphicsserver.h"
#include "basegamefeature/managers/timemanager.h"
#include "physicsfeature/managers/physicsmanager.h"
#include "physics/actorcontext.h"
#include "game/api.h"

namespace PhysicsFeature
{
__ImplementClass(PhysicsFeature::PhysicsFeatureUnit, 'PXFU' , Game::FeatureUnit);
__ImplementSingleton(PhysicsFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
PhysicsFeatureUnit::PhysicsFeatureUnit()
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
PhysicsFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    Game::TimeSourceCreateInfo timeSourceInfo;
    timeSourceInfo.hash = TIMESOURCE_PHYSICS;
    Game::TimeManager::CreateTimeSource(timeSourceInfo);

    this->AttachManager(PhysicsManager::Create());

    Physics::Setup();
    Physics::SetOnSleepCallback([](Physics::ActorId* actors, SizeT num)
    {
        // FIXME: This assumes all actors are in the default world.
        //        This might not be true and we should allow multiple physics stages as well.
        Game::World* world = Game::GetWorld(WORLD_DEFAULT);
        Game::ComponentId staticPid = Game::GetComponentId("Static"_atm);
        Game::OpBuffer buffer = Game::CreateOpBuffer(world);
        for (IndexT i = 0; i < num; i++)
        {
            Physics::Actor& actor = Physics::ActorContext::GetActor(actors[i]);
            Game::Entity entity = Game::Entity::FromId((Ids::Id32)actor.userData);
            Game::Op::RegisterComponent registerOp;
            registerOp.entity = entity;
            registerOp.component = staticPid;
            registerOp.value = nullptr; // no value since it's a flag property
            Game::AddOp(buffer, registerOp);
        }
        Game::Dispatch(buffer);
        Game::DestroyOpBuffer(buffer);
    });
    Physics::SetOnWakeCallback([](Physics::ActorId* actors, SizeT num)
    {
        // FIXME: This assumes all actors are in the default world.
        //        This might not be true and we should allow multiple physics stages as well.
        Game::World* world = Game::GetWorld(WORLD_DEFAULT);
        Game::ComponentId staticPid = Game::GetComponentId("Static"_atm);
        Game::OpBuffer buffer = Game::CreateOpBuffer(world);
        for (IndexT i = 0; i < num; i++)
        {
            Physics::Actor& actor = Physics::ActorContext::GetActor(actors[i]);
            Game::Entity entity = Game::Entity::FromId((Ids::Id32)actor.userData);
            if (Game::HasComponent(world, entity, staticPid))
            {
                Game::Op::DeregisterComponent deregisterOp;
                deregisterOp.entity = entity;
                deregisterOp.component = staticPid;
                Game::AddOp(buffer, deregisterOp);
            }
        }
        Game::Dispatch(buffer);
        Game::DestroyOpBuffer(buffer);
    });
    Physics::CreateScene();
    //FIXME
    IndexT dummyMaterial = Physics::CreateMaterial("dummy"_atm, 0.8, 0.6, 0.3, 1.0);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnDeactivate()
{   
    FeatureUnit::OnDeactivate();    
    Physics::ShutDown();
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsFeatureUnit::OnBeginFrame()
{
    Game::TimeSource* const time = Game::TimeManager::GetTimeSource(TIMESOURCE_PHYSICS);
    Physics::Update(time->frameTime);
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
