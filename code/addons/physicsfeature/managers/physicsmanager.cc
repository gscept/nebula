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
#include "components/physics.h"
#include "graphicsfeature/graphicsfeatureunit.h"

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
    Game::FilterBuilder::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetComponentId("Owner");
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.inclusive[1] = Game::GetComponentId("WorldTransform");
    filterInfo.access[1] = Game::AccessMode::READ;
    filterInfo.inclusive[2] = Game::GetComponentId("PhysicsResource");
    filterInfo.access[2] = Game::AccessMode::READ;
    filterInfo.numInclusive = 3;

    filterInfo.exclusive[0] = this->pids.physicsActor;
    filterInfo.numExclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "PhysicsManager.CreateActors"_atm;
    processorInfo.OnBeginFrame = [](Game::World* world, Game::Dataset data)
    {
        Game::OpBuffer opBuffer = Game::CreateOpBuffer(world);
        Game::ComponentId const modelID = Game::GetComponentId("ModelResource"_atm);
        Game::ComponentId const typeID = Game::GetComponentId("PhysicsType"_atm);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];
            Game::Entity const* const owners = (Game::Entity*)view.buffers[0];
            Math::mat4 const* const transforms = (Math::mat4*)view.buffers[1];
            Resources::ResourceName const* const resources = (Resources::ResourceName*)view.buffers[2];
            

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Game::Entity const& entity = owners[i];
                Math::mat4 const& trans = transforms[i];
                Resources::ResourceName res = resources[i];
                
                if (res == "mdl:")
                {
                    Util::String modelRes = Game::GetComponent<GraphicsFeature::ModelResource>(world, entity, modelID).value.Value();
                    Util::String fileName = modelRes.ExtractFileName();
                    fileName.StripFileExtension();
                    res = Util::String::Sprintf("phys:%s/%s.actor", modelRes.ExtractLastDirName().AsCharPtr(), fileName.AsCharPtr());
                }

                Physics::ActorType actorType = Physics::ActorType::Static;
                if (Game::HasComponent(world, entity, typeID))
                {
                    actorType = Game::GetComponent<Physics::ActorType>(world, entity, typeID);
                }

                Resources::CreateResource(res, "PHYS",
                    [world, trans, &opBuffer, entity, actorType](Resources::ResourceId id)
                    {
                        
                        Physics::ActorId actorid = Physics::CreateActorInstance(id, trans, actorType, Ids::Id32(entity));
                        
                        Game::Op::RegisterComponent regOp;
                        regOp.entity = entity;
                        regOp.component = PhysicsManager::Singleton->pids.physicsActor;
                        regOp.value = &actorid;
                        Game::AddOp(opBuffer, regOp);

                    }, nullptr, true);
            }
        }

        // execute ops
        Game::Dispatch(opBuffer);
        Game::DestroyOpBuffer(opBuffer);
    };
    
    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnDecay()
{
    Game::ComponentDecayBuffer const decayBuffer = Game::GetDecayBuffer(Singleton->pids.physicsActor);
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
    Game::FilterBuilder::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = this->pids.physicsActor;
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.inclusive[1] = Game::GetComponentId("WorldTransform"_atm);
    filterInfo.access[1] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 2;

    filterInfo.exclusive[0] = Game::GetComponentId("Static");
    filterInfo.numExclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);

    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = "PhysicsManager.PollTransforms"_atm;
    processorInfo.OnFrame = [](Game::World* world, Game::Dataset data)
    {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];
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
    PhysicsManager::Singleton = new PhysicsManager;
    
    Game::ComponentCreateInfo info;
    info.name = "PhysicsActorId";
    Physics::ActorId defaultValue;
    info.defaultValue = &defaultValue;
    info.flags = Game::ComponentFlags::COMPONENTFLAG_MANAGED;
    info.byteSize = sizeof(Physics::ActorId);
    Singleton->pids.physicsActor = Game::CreateComponent(info);

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
    filterInfo.inclusive[0] = Singleton->pids.physicsActor;
    filterInfo.access[0] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::Dataset data = Game::Query(world, filter);
    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::EntityTableView const& view = data.views[v];
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


#include "pjson/pjson.h"
#include "io/jsonreader.h"

namespace IO
{
     template<>  void JsonReader::Get<Physics::ActorType>(Physics::ActorType& ret, char const* attr)
    {
         ret = Physics::ActorType::Static;
        const pjson::value_variant* node = this->GetChild(attr);
        if (node->is_string())
        {
            Util::String str = node->as_string_ptr();
            if (str == "Static") { ret = Physics::ActorType::Static; return; }
            if (str == "Kinematic") { ret = Physics::ActorType::Kinematic; return; }
            if (str == "Dynamic") { ret = Physics::ActorType::Dynamic; return; }
        }
    }

    template<>  void JsonWriter::Add<Physics::ActorType>(Physics::ActorType const& t, Util::String const& val)
    {
        switch (t)
        {
            case Physics::ActorType::Static: this->Add("Static", val); return;
            case Physics::ActorType::Kinematic: this->Add("Kinematic", val); return;
            case Physics::ActorType::Dynamic: this->Add("Dynamic", val); return;
        }
    }
};
