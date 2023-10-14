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

//------------------------------------------------------------------------------
/**

All entities get IsActive flag at beginning of frame
Full partition migration from one table to the one that has the flag

OnActivate is run before flag is set, automatically filtered on !IsActive

*/

namespace PhysicsFeature
{

__ImplementSingleton(PhysicsManager)

    //DEFINE_COMPONENT(PhysicsActorT);

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
               Game::OwnerT const& owner,
               Game::WorldTransformT const& transform,
               PhysicsFeature::PhysicsActorT& actor)
            {
                auto res = actor.resource;
                if (res == "mdl:")
                {
                    n_assert(Game::HasComponent(world, owner.value, GraphicsFeature::Model::ID()));
                    Util::String modelRes = Game::GetComponent<GraphicsFeature::Model>(world, owner.value).resource.Value();
                    Util::String fileName = modelRes.ExtractFileName();
                    fileName.StripFileExtension();
                    res = Util::String::Sprintf(
                        "phys:%s/%s.actor", modelRes.ExtractLastDirName().AsCharPtr(), fileName.AsCharPtr()
                    );
                    actor.resource = res;
                }

                Resources::ResourceId resId = Resources::CreateResource(res, "PHYS", nullptr, nullptr, true);
                Physics::ActorId actorid =
                    Physics::CreateActorInstance(resId, transform.value, actor.actorType, Ids::Id32(owner.value));
                actor.value = actorid;

                if (actor.actorType == Physics::ActorType::Kinematic)
                {
                    Game::Op::RegisterComponent regOp;
                    regOp.entity = owner.value;
                    regOp.component = PhysicsFeature::IsKinematic::ID();
                    regOp.value = nullptr;
                    Game::AddOp(Game::WorldGetScratchOpBuffer(world), regOp);
                }
            }
        )
        .Build();

    //Game::ProcessorBuilder("PhysicsManager.CreateActors"_atm)
    //    .Excluding<PhysicsActorT>()
    //    .On("OnBeginFrame")
    //    .Func(&CreateActor)
    //    .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnDecay()
{
    Game::ComponentDecayBuffer const decayBuffer = Game::GetDecayBuffer(PhysicsActorT::ID());
    PhysicsFeature::PhysicsActorT* data = (PhysicsFeature::PhysicsActorT*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        Physics::DestroyActorInstance(data[i].value);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PollRigidbodyTransforms(Game::World* world, Game::WorldTransformT& transform, PhysicsFeature::PhysicsActorT const& actor)
{
    transform.value = Physics::ActorContext::GetTransform(actor.value);
}

//------------------------------------------------------------------------------
/**
*/
void
PassKinematicTransforms(
    Game::World* world, Game::WorldTransformT const& transform, PhysicsFeature::PhysicsActorT& actor, PhysicsFeature::IsKinematic
)
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
        .Excluding({Game::GetComponentId("Static"), IsKinematic::ID()})
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
    n_assert(PhysicsFeature::Details::physics_registered);
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
    filterInfo.inclusive[0] = PhysicsActorT::ID();
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
template <>
void
JsonReader::Get<Physics::ActorType>(Physics::ActorType& ret, char const* attr)
{
    ret = Physics::ActorType::Static;
    const pjson::value_variant* node = this->GetChild(attr);
    if (node->is_string())
    {
        Util::String str = node->as_string_ptr();
        if (str == "Static")
        {
            ret = Physics::ActorType::Static;
            return;
        }
        if (str == "Kinematic")
        {
            ret = Physics::ActorType::Kinematic;
            return;
        }
        if (str == "Dynamic")
        {
            ret = Physics::ActorType::Dynamic;
            return;
        }
    }
}

template <>
void
JsonWriter::Add<Physics::ActorType>(Physics::ActorType const& t, Util::String const& val)
{
    switch (t)
    {
    case Physics::ActorType::Static:
        this->Add("Static", val);
        return;
    case Physics::ActorType::Kinematic:
        this->Add("Kinematic", val);
        return;
    case Physics::ActorType::Dynamic:
        this->Add("Dynamic", val);
        return;
    }
}

template <>
void
JsonReader::Get<Physics::ActorId>(Physics::ActorId& ret, char const* attr)
{
    // read nothing
    ret = Physics::ActorId();
}

template <>
void
JsonWriter::Add<Physics::ActorId>(Physics::ActorId const& id, Util::String const& val)
{
    // Write nothing
    return;
}

}; // namespace IO
