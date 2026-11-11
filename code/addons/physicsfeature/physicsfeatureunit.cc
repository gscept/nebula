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
#include "game/world.h"
#include "components/physicsfeature.h"
#include "managers/charactermanager.h"

#define USE_SYNC_UPDATE 0

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
PhysicsFeatureUnit::OnAttach()
{
    // the handle component owns a physx actor and queues its setup on init. The body and collider
    // components ensure a handle on init and release their actor and shapes on decay, everything in
    // between setup and teardown is a change, and changes are pinged by whoever made them
    this->RegisterComponentType<PhysicsActorId>({ .decay = true, .OnInit = &PhysicsManager::InitPhysicsActorId });
    this->RegisterComponentType<IsKinematic>();
    this->RegisterComponentType<Character>({ .decay = true });

    this->RegisterComponentType<PhysicsActor>({ .OnInit = &PhysicsManager::InitPhysicsActor, .OnDecay = &PhysicsManager::OnBodyComponentDecay<PhysicsFeature::PhysicsActor> });
    this->RegisterComponentType<Rigidbody>({ .OnInit = &PhysicsManager::InitRigidbody, .OnDecay = &PhysicsManager::OnBodyComponentDecay<PhysicsFeature::Rigidbody> });
    this->RegisterComponentType<StaticBody>({ .OnInit = &PhysicsManager::InitStaticBody, .OnDecay = &PhysicsManager::OnBodyComponentDecay<PhysicsFeature::StaticBody> });
    this->RegisterComponentType<BoxCollider>({ .OnInit = &PhysicsManager::InitBoxCollider, .OnDecay = &PhysicsManager::OnColliderComponentDecay<PhysicsFeature::BoxCollider> });
    this->RegisterComponentType<SphereCollider>({ .OnInit = &PhysicsManager::InitSphereCollider, .OnDecay = &PhysicsManager::OnColliderComponentDecay<PhysicsFeature::SphereCollider> });
    this->RegisterComponentType<CapsuleCollider>({ .OnInit = &PhysicsManager::InitCapsuleCollider, .OnDecay = &PhysicsManager::OnColliderComponentDecay<PhysicsFeature::CapsuleCollider> });
    this->RegisterComponentType<PlaneCollider>({ .OnInit = &PhysicsManager::InitPlaneCollider, .OnDecay = &PhysicsManager::OnColliderComponentDecay<PhysicsFeature::PlaneCollider> });
    this->RegisterComponentType<MeshCollider>({ .OnInit = &PhysicsManager::InitMeshCollider, .OnDecay = &PhysicsManager::OnColliderComponentDecay<PhysicsFeature::MeshCollider> });
    this->RegisterComponentType<HeightFieldCollider>({ .OnInit = &PhysicsManager::InitHeightFieldCollider, .OnDecay = &PhysicsManager::OnColliderComponentDecay<PhysicsFeature::HeightFieldCollider> });
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
    this->AttachManager(CharacterManager::Create());

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

        // this is where managers destroy their actors, which has to happen before the scene goes away
        FeatureUnit::OnBeforeCleanup(world);

        // destroy the scene with the world
        Physics::DestroyScene(scene);
        this->physicsWorlds.Erase(world);
    }
}

//------------------------------------------------------------------------------
/**
*/
IndexT
PhysicsFeatureUnit::SceneForWorld(Game::World* world)
{
    if (!this->physicsWorlds.Contains(world))
    {
        this->physicsWorlds.Add(world, Physics::CreateScene());
    }
    return this->physicsWorlds[world];
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsFeatureUnit::OnBeginFrame()
{
#if USE_SYNC_UPDATE > 0
    Game::TimeSource* const time = Game::Time::GetTimeSource(TIMESOURCE_PHYSICS);
    for (auto const& scene : this->physicsWorlds)
    {
        if (Game::PhaseForMode(scene.Key()->mode) != Game::PlayPhase)
            continue;

        Physics::Update(time->frameTime, scene.Value());
    }
#else
    for (auto const& scene : this->physicsWorlds)
    {
        Physics::EndSimulating(scene.Value());
    }
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
        if (Game::PhaseForMode(scene.Key()->mode) != Game::PlayPhase)
            continue;

        Physics::BeginSimulating(time->frameTime, scene.Value());
    }
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
