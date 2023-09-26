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
#include "basegamefeature/components/transform.h"

namespace PhysicsFeature
{

__ImplementSingleton(PhysicsManager)

struct PhysicsActor
{
    Physics::ActorId value;
    DECLARE_COMPONENT;
};

DEFINE_COMPONENT(PhysicsActor);

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
void CreateActor(Game::World* world, Game::Owner const& entity, Game::WorldTransform const& trans, PhysicsFeature::PhysicsResource& res)
{
    if (res.value == "mdl:")
    {
        n_assert(Game::HasComponent(world, entity.value, GraphicsFeature::ModelResource::ID()));
        Util::String modelRes = Game::GetComponent<GraphicsFeature::ModelResource>(world, entity.value).value.Value();
        Util::String fileName = modelRes.ExtractFileName();
        fileName.StripFileExtension();
        res.value = Util::String::Sprintf("phys:%s/%s.actor", modelRes.ExtractLastDirName().AsCharPtr(), fileName.AsCharPtr());
    }

    Physics::ActorType actorType = Physics::ActorType::Static;
    if (Game::HasComponent(world, entity.value, PhysicsFeature::PhysicsType::ID()))
    {
        actorType = Game::GetComponent<PhysicsFeature::PhysicsType>(world, entity.value).value;
    }
    
    Resources::CreateResource(res.value, "PHYS",
        [world, trans, entity, actorType](Resources::ResourceId id)
        {

            Physics::ActorId actorid = Physics::CreateActorInstance(id, trans.value, actorType, Ids::Id32(entity.value));

            Game::Op::RegisterComponent regOp;
            regOp.entity = entity.value;
            regOp.component = PhysicsActor::ID();
            regOp.value = &actorid;
            Game::AddOp(Game::WorldGetScratchOpBuffer(world), regOp);

            if (actorType == Physics::ActorType::Kinematic)
            {
                Game::Op::RegisterComponent regOp;
                regOp.entity = entity.value;
                regOp.component = PhysicsFeature::IsKinematic::ID();
                regOp.value = nullptr;
                Game::AddOp(Game::WorldGetScratchOpBuffer(world), regOp);
            }
        }, nullptr, true);
}

//------------------------------------------------------------------------------
/**
*/
void PhysicsManager::InitCreateActorProcessor()
{
    Game::ProcessorBuilder("PhysicsManager.CreateActors"_atm)
        .Excluding<PhysicsActor>()
        .On("OnBeginFrame")
        .Func(&CreateActor)
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnDecay()
{
    Game::ComponentDecayBuffer const decayBuffer = Game::GetDecayBuffer(PhysicsActor::ID());
    Physics::ActorId* data = (Physics::ActorId*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        Physics::DestroyActorInstance(data[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PollRigidbodyTransforms(Game::World* world, Game::WorldTransform& transform, PhysicsFeature::PhysicsActor const& actor)
{
    transform.value = Physics::ActorContext::GetTransform(actor.value);
}

//------------------------------------------------------------------------------
/**
*/
void
PassKinematicTransforms(Game::World* world, Game::WorldTransform const& transform, PhysicsFeature::PhysicsActor const& actor, PhysicsFeature::IsKinematic)
{
    Physics::ActorContext::SetTransform(actor.value, transform.value);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPollTransformProcessor()
{
    Game::ProcessorBuilder("PhysicsManager.PollRigidbodyTransforms"_atm)
        .Excluding({ Game::GetComponentId("Static"), IsKinematic::ID() })
        .On("OnFrame")
        .Func(&PollRigidbodyTransforms)
        .Build();

    Game::ProcessorBuilder("PhysicsManager.PassKinematicTransforms"_atm)
        .Excluding({ Game::GetComponentId("Static") })
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
    n_assert(PhysicsFeature::Details::physics_registered);
    n_assert(!PhysicsManager::HasInstance());
    PhysicsManager::Singleton = new PhysicsManager;

    MemDb::TypeRegistry::Register<PhysicsActor>("PhysicsActorId"_atm, PhysicsActor(), Game::ComponentFlags::COMPONENTFLAG_MANAGED);

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
    filterInfo.inclusive[0] = PhysicsActor::ID();
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
