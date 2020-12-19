//------------------------------------------------------------------------------
//  physicsmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "physicsmanager.h"
#include "basegamefeature/managers/entitymanager.h"
#include "game/gameserver.h"
#include "physicsinterface.h"
#include "physics/streamactorpool.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "physics/utils.h"

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
MoveCallback(Physics::ActorId id, Math::mat4 const& trans)
{
    static Game::PropertyId const transformPropertyId = Game::GetPropertyId("WorldTransform");
    Physics::Actor& actor = Physics::ActorContext::GetActor(id);
    Game::Entity gameId = (Game::Entity)actor.userData;
    Game::SetProperty(gameId, transformPropertyId, trans);
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
    processorInfo.OnBeginFrame = [](Game::Dataset data)
    {
        Game::OpBuffer opBuffer = Game::CreateOpBuffer();
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
                    [trans, &opBuffer, entity, staticPID](Resources::ResourceId id)
                    {
                        bool const dynamic = !Game::HasProperty(entity, staticPID);
                        Physics::ActorId actorid = Physics::CreateActorInstance(id, trans, dynamic);
                        Physics::Actor& actor = Physics::ActorContext::GetActor(actorid);
                        actor.userData = entity.id;
                        //actor.moveCallback = Util::Delegate<void(Physics::ActorId, Math::mat4 const&)>::FromFunction<&MoveCallback>();

                        Game::Op::RegisterProperty regOp;
                        regOp.entity = entity;
                        regOp.pid = PhysicsManager::Singleton->pids.physicsActor;
                        regOp.value = &actorid;
                        Game::AddOp(opBuffer, regOp);

                    }, [&res](Resources::ResourceId id)
                    {
                        n_warning("Failed to load physics actor from %s\n", res.Value());
                    }, true);
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
void PhysicsManager::InitDestroyActorProcessor()
{
    Game::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = this->pids.physicsActor;
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.numInclusive = 1;

    filterInfo.exclusive[0] = Game::GetPropertyId("PhysicsResource");
    filterInfo.numExclusive = 1;

    Game::Filter filter = Game::CreateFilter(filterInfo);

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "PhysicsManager.DestroyActors"_atm;
    processorInfo.OnBeginFrame = [](Game::Dataset data)
    {
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
    };

    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
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
    processorInfo.OnFrame = [](Game::Dataset data)
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
    n_assert(!PhysicsManager::HasInstance());
    PhysicsManager::Singleton = n_new(PhysicsManager);
    
    Singleton->pids.physicsActor = MemDb::TypeRegistry::Register("PhysicsActorId"_atm, Physics::ActorId(), Game::PropertyFlags::PROPERTYFLAG_MANAGED);

    Singleton->InitCreateActorProcessor();
    Singleton->InitDestroyActorProcessor();
    Singleton->InitPollTransformProcessor();

    Game::ManagerAPI api;
    api.OnBeginFrame = &OnBeginFrame;
    api.OnDeactivate = &Destroy;
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
PhysicsManager::OnBeginFrame()
{
    n_assert(PhysicsManager::HasInstance());
}

} // namespace PhysicsFeature
