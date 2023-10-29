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
PhysicsManager::InitCreateActorProcessor()
{
    Game::ProcessorBuilder("PhysicsManager.CreateActors"_atm)
        .On("OnActivate")
        .Func(
            [](Game::World* world,
               Game::Owner const& owner,
               Game::Transform const& transform,
               PhysicsFeature::PhysicsActor& actor)
            {
                auto res = actor.resource;
                if (res == "mdl:")
                {
                    n_assert(world->HasComponent(owner.entity, Game::GetComponentId<GraphicsFeature::Model>()));
                    Util::String modelRes = world->GetComponent<GraphicsFeature::Model>(owner.entity).resource.Value();
                    Util::String fileName = modelRes.ExtractFileName();
                    fileName.StripFileExtension();
                    res = Util::String::Sprintf(
                        "phys:%s/%s.actor", modelRes.ExtractLastDirName().AsCharPtr(), fileName.AsCharPtr()
                    );
                    actor.resource = res;
                }

                Resources::ResourceId resId = Resources::CreateResource(res, "PHYS", nullptr, nullptr, true);
                Physics::ActorId actorid =
                    Physics::CreateActorInstance(resId, transform.value, (Physics::ActorType)actor.actorType, Ids::Id32(owner.entity));
                actor.actorId = actorid.id;

                if (actor.actorType == Physics::ActorType::Kinematic)
                {
                    world->AddComponent<PhysicsFeature::IsKinematic>(owner.entity);
                }
            }
        )
        .Build();
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
PollRigidbodyTransforms(Game::World* world, Game::Transform& transform, PhysicsFeature::PhysicsActor const& actor)
{
    transform.value = Physics::ActorContext::GetTransform(actor.actorId);
}

//------------------------------------------------------------------------------
/**
*/
void
PassKinematicTransforms(
    Game::World* world, Game::Transform const& transform, PhysicsFeature::PhysicsActor& actor, PhysicsFeature::IsKinematic
)
{
    Physics::ActorContext::SetTransform(actor.actorId, transform.value);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPollTransformProcessor()
{
    Game::ProcessorBuilder("PhysicsManager.PollRigidbodyTransforms"_atm)
        .Excluding({Game::GetComponentId("Static"), Game::GetComponentId<IsKinematic>()})
        .On("OnFrame")
        .Func(&PollRigidbodyTransforms)
        .Build();

    Game::ProcessorBuilder("PhysicsManager.PassKinematicTransforms"_atm)
        .Excluding({Game::GetComponentId("Static")})
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

    Singleton->InitCreateActorProcessor();
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
