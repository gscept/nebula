//------------------------------------------------------------------------------
//  physicsmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "physicsmanager.h"
#include "game/gameserver.h"
#include "physicsinterface.h"
#include "physics/streamactorpool.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "physics/utils.h"
#include "properties/physics.h"

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
void PhysicsManager::InitCreateActorProcessor()
{
    Game::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetPropertyId("Owner");
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.inclusive[1] = Game::GetPropertyId("WorldTransform");
    filterInfo.access[1] = Game::AccessMode::READ;
    filterInfo.inclusive[2] = Game::GetPropertyId("PhysicsResource");
    filterInfo.access[2] = Game::AccessMode::READ;
    filterInfo.numInclusive = 3;

    filterInfo.exclusive[0] = this->pids.physicsActor;
    filterInfo.numExclusive = 1;

    Game::Filter filter = Game::CreateFilter(filterInfo);

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "PhysicsManager.CreateActors"_atm;
    processorInfo.OnBeginFrame = [](Game::World* world, Game::Dataset data)
    {
        Game::OpBuffer opBuffer = Game::CreateOpBuffer(world);
        Game::PropertyId const staticPID = Game::GetPropertyId("Static"_atm);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::CategoryTableView const& view = data.views[v];
            Game::Entity const* const owners = (Game::Entity*)view.buffers[0];
            Math::mat4 const* const transforms = (Math::mat4*)view.buffers[1];
            Resources::ResourceName const* const resources = (Resources::ResourceName*)view.buffers[2];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Game::Owner const& entity = owners[i];
                Math::mat4 const& trans = transforms[i];
                Resources::ResourceName const& res = resources[i];

                Resources::CreateResource(res, "PHYS",
                    [world, trans, &opBuffer, entity, staticPID](Resources::ResourceId id)
                    {
                        bool const dynamic = !Game::HasProperty(world, entity, staticPID);
                        Physics::ActorId actorid = Physics::CreateActorInstance(id, trans, dynamic);
                        Physics::Actor& actor = Physics::ActorContext::GetActor(actorid);
                        actor.userData = Ids::Id32(entity);
                        
                        Game::Op::RegisterProperty regOp;
                        regOp.entity = entity;
                        regOp.pid = PhysicsManager::Singleton->pids.physicsActor;
                        regOp.value = &actorid;
                        Game::AddOp(opBuffer, regOp);

                    }, nullptr, true);
            }
        }

        // execute ops
        Game::Dispatch(opBuffer);
    };
    
    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnDecay()
{
    Game::PropertyDecayBuffer const decayBuffer = Game::GetDecayBuffer(Singleton->pids.physicsActor);
    Physics::ActorId* data = (Physics::ActorId*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        Physics::DestroyActorInstance(data[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void PhysicsManager::InitPollTransformProcessor()
{
    Game::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = this->pids.physicsActor;
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.inclusive[1] = Game::GetPropertyId("WorldTransform"_atm);
    filterInfo.access[1] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 2;

    filterInfo.exclusive[0] = Game::GetPropertyId("Static");
    filterInfo.numExclusive = 1;

    Game::Filter filter = Game::CreateFilter(filterInfo);

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "PhysicsManager.PollTransforms"_atm;
    processorInfo.OnFrame = [](Game::World* world, Game::Dataset data)
    {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::CategoryTableView const& view = data.views[v];
            Physics::ActorId const* const actorIdBuffer = (Physics::ActorId*)view.buffers[0];
            Math::mat4* const transforms = (Math::mat4*)view.buffers[1];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Physics::ActorId const actorid = actorIdBuffer[i];
                Math::mat4& transform = transforms[i];
                transform = Physics::ActorContext::GetTransform(actorid);
            }
        }
    };

    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
PhysicsManager::Create()
{
	n_assert(PhysicsFeature::Details::physics_registered);
	n_assert(!PhysicsManager::HasInstance());
    PhysicsManager::Singleton = n_new(PhysicsManager);
    
    Game::PropertyCreateInfo info;
    info.name = "PhysicsActorId";
    Physics::ActorId defaultValue;
    info.defaultValue = &defaultValue;
    info.flags = Game::PropertyFlags::PROPERTYFLAG_MANAGED;
    info.byteSize = sizeof(Physics::ActorId);
    Singleton->pids.physicsActor = Game::CreateProperty(info);

    Singleton->InitCreateActorProcessor();
    Singleton->InitPollTransformProcessor();

    Game::ManagerAPI api;
    api.OnCleanup    = &OnCleanup;
    api.OnDeactivate = &Destroy;
    api.OnDecay      = &OnDecay;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::Destroy()
{
    n_delete(PhysicsManager::Singleton);
    PhysicsManager::Singleton = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnCleanup(Game::World* world)
{
    n_assert(PhysicsManager::HasInstance());
    Game::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Singleton->pids.physicsActor;
    filterInfo.access[0] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::CreateFilter(filterInfo);
    Game::Dataset data = Game::Query(world, filter);
    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::CategoryTableView const& view = data.views[v];
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
