//------------------------------------------------------------------------------
//  physicsmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "physicsmanager.h"
#include "game/gameserver.h"
#include "physicsinterface.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "components/physicsfeature.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "basegamefeature/components/velocity.h"

namespace PhysicsFeature
{

__ImplementSingleton(PhysicsManager)

//------------------------------------------------------------------------------
/**
*/
PhysicsManager::PhysicsManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
PhysicsManager::~PhysicsManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPhysicsActor(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor* actor)
{
    auto res = actor->resource;
    if (res == "mdl:")
    {
        n_assert(world->HasComponent<GraphicsFeature::Model>(entity));
        Util::String modelRes = world->GetComponent<GraphicsFeature::Model>(entity).resource.Value();
        Util::String fileName = modelRes.ExtractFileName();
        fileName.StripFileExtension();
        res = Util::String::Sprintf(
            "phys:%s/%s.actor", modelRes.ExtractLastDirName().AsCharPtr(), fileName.AsCharPtr()
        );
        actor->resource = res;
    }

    Math::transform worldTransform = Math::transform(
        world->GetComponent<Game::Position>(entity),
        world->GetComponent<Game::Orientation>(entity),
        world->GetComponent<Game::Scale>(entity)
    );

    Resources::ResourceId resId = Resources::CreateResource(res, "PHYS", nullptr, nullptr, true);
    Physics::ActorId actorid =
        Physics::CreateActorInstance(resId, worldTransform, (Physics::ActorType)actor->actorType, Ids::Id32(entity));
    actor->actorId = actorid.id;

    if (actor->actorType == Physics::Kinematic)
    {
        world->AddComponent<PhysicsFeature::IsKinematic>(entity);
    }
    if (world->HasComponent<Game::Velocity>(entity))
    {
        Physics::ActorContext::SetLinearVelocity(actorid, world->GetComponent<Game::Velocity>(entity));
    }
    if (world->HasComponent< Game::AngularVelocity>(entity))
    {
        Physics::ActorContext::SetAngularVelocity(actorid, world->GetComponent<Game::AngularVelocity>(entity));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnDecay()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::ComponentDecayBuffer const decayBuffer = world->GetDecayBuffer(Game::GetComponentId<PhysicsActor>());
    PhysicsFeature::PhysicsActor* data = (PhysicsFeature::PhysicsActor*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        Physics::DestroyActorInstance(data[i].actorId);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PollRigidbodyTransforms(Game::World* world, Game::Position& position, Game::Orientation& orientation, PhysicsFeature::PhysicsActor const& actor)
{
    Physics::ActorContext::GetPositionOrientation(actor.actorId, position, orientation);
}

//------------------------------------------------------------------------------
/**
*/
void
PassKinematicTransforms(
    Game::World* world, Game::Position const& position, Game::Orientation const& orientation, PhysicsFeature::PhysicsActor& actor, PhysicsFeature::IsKinematic
)
{
    Physics::ActorContext::SetPositionOrientation(actor.actorId, position, orientation);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPollTransformProcessor()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::ProcessorBuilder(world, "PhysicsManager.PollRigidbodyTransforms"_atm)
        .Excluding<Game::Static, IsKinematic>()
        .On("OnFrame")
        .Func(&PollRigidbodyTransforms)
        .Build();

    Game::ProcessorBuilder(world, "PhysicsManager.PassKinematicTransforms"_atm)
        .Excluding<Game::Static>()
        .On("OnFrame")
        .Func(&PassKinematicTransforms)
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
PhysicsManager::Create()
{
    n_assert(!PhysicsManager::HasInstance());
    PhysicsManager::Singleton = new PhysicsManager;

    Singleton->InitPollTransformProcessor();

    Game::ManagerAPI api;
    api.OnCleanup = &OnCleanup;
    api.OnDeactivate = &Destroy;
    api.OnDecay = &OnDecay;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::Destroy()
{
    delete PhysicsManager::Singleton;
    PhysicsManager::Singleton = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnCleanup(Game::World* world)
{
    n_assert(PhysicsManager::HasInstance());

    Game::FilterBuilder::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetComponentId<PhysicsActor>();
    filterInfo.access[0] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::Dataset data = world->Query(filter);
    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        Physics::ActorId* const actors = (Physics::ActorId*)view.buffers[0];

        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            Physics::ActorId const& actorid = actors[i];
            Physics::DestroyActorInstance(actorid);
        }
    }

    Game::DestroyFilter(filter);
}

} // namespace PhysicsFeature
