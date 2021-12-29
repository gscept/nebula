//------------------------------------------------------------------------------
//  physicsinterface.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxPhysicsAPI.h"
#include "physicsinterface.h"
#include "physics/utils.h"
#include "physics/actorcontext.h"
#include "math/scalar.h"
#include "timing/time.h"
#include "physics/physxstate.h"
#include "physics/streamactorpool.h"
#include "resources/resourceserver.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "nflatbuffer/flatbufferinterface.h"

#define PHYSX_MEMORY_ALLOCATION_DEBUG false
#define PHYSX_THREADS 2

using namespace physx;
using namespace Physics;

namespace
{
/// Physx simulation filter
PxFilterFlags Simulationfilter(PxFilterObjectAttributes attributes0,
                               PxFilterData                filterData0,
                               PxFilterObjectAttributes    attributes1,
                               PxFilterData                filterData1,
                               PxPairFlags& pairFlags,
                               const void* constantBlock,
                               PxU32                       constantBlockSize)
{
    PxFilterFlags filterFlags = PxDefaultSimulationFilterShader(attributes0,
                                                                filterData0, attributes1, filterData1, pairFlags, constantBlock, constantBlockSize);
    if (pairFlags & PxPairFlag::eSOLVE_CONTACT)
    {
        if (filterData0.word1 & CollisionFeedback_Full || filterData1.word1 & CollisionFeedback_Full)
        {
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eDETECT_DISCRETE_CONTACT | PxPairFlag::eNOTIFY_CONTACT_POINTS;
        }
        else if (filterData0.word1 & CollisionFeedback_Single || filterData1.word1 & CollisionFeedback_Single)
        {
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eDETECT_DISCRETE_CONTACT | PxPairFlag::eNOTIFY_CONTACT_POINTS;
        }
    }
    return filterFlags;
}
};

namespace Physics
{

void LoadMaterialTable();
    //------------------------------------------------------------------------------
/**
*/
void 
Setup()
{
    state.Setup();
    Resources::ResourceServer::Instance()->RegisterStreamPool("actor", Physics::StreamActorPool::RTTI);
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("phys","export:physics"));

    Physics::actorPool = Resources::GetStreamPool<Physics::StreamActorPool>();
    LoadMaterialTable();

    //FIXME this should be somewhere in a toolkit component instead
    Flat::FlatbufferInterface::Init();
}

//------------------------------------------------------------------------------
/**
*/
void ShutDown()
{
    PxCloseExtensions();
    state.cooking->release();
    state.physics->release();
    state.foundation->release();
}

//------------------------------------------------------------------------------
/**
*/
IndexT
CreateScene()
{
    n_assert(state.foundation);
    IndexT idx = -1;
    if (!state.deadSceneIds.IsEmpty())
    {
        idx = state.deadSceneIds.Back();
        state.deadSceneIds.EraseBack();
    }
    else
    {
        idx = state.activeScenes.Size();
    }
    state.activeSceneIds.Append(idx);
    state.activeScenes.Append(Scene());
    Scene & scene = state.activeScenes[idx];
    scene.dispatcher = PxDefaultCpuDispatcherCreate(PHYSX_THREADS);

    PxSceneDesc sceneDesc(state.physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = scene.dispatcher;
    sceneDesc.filterShader = Simulationfilter;
    scene.scene = state.physics->createScene(sceneDesc);    
    scene.scene->setSimulationEventCallback(&state);    
    scene.controllerManager= PxCreateControllerManager(*scene.scene);       
#ifdef NEBULA_DEBUG
    scene.scene->getScenePvdClient()->setScenePvdFlags(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS | PxPvdSceneFlag::eTRANSMIT_CONTACTS | PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES );
#endif
    scene.physics = state.physics;
    scene.foundation = state.foundation;
    return idx;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyScene(IndexT sceneId)
{
    n_assert(state.foundation);
    n_assert(sceneId < state.activeScenes.Size() && state.activeSceneIds.FindIndex(sceneId) != InvalidIndex);

    Scene& scene = state.activeScenes[sceneId];

    auto actorCount = scene.scene->getNbActors(PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC);
    if (actorCount > 0)
    {
        n_warn("Destroying non-empty physics scene");
        PxU32 bufferSize;
        PxActor** actorBuffer = (PxActor**)Memory::Alloc(Memory::PhysicsHeap, bufferSize * sizeof(PxActor*));
        scene.scene->getActors(PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC, actorBuffer, bufferSize);
        for (int i = 0; i < actorCount; i++)
        {
            actorBuffer[i]->release();
        }
        Memory::Free(Memory::PhysicsHeap, actorBuffer);
    }
    scene.controllerManager->release();
    scene.scene->release();
    scene.dispatcher->release();
    state.deadSceneIds.Append(sceneId);
}

//------------------------------------------------------------------------------
/**
*/
Physics::Scene &
GetScene(IndexT idx)
{
    n_assert(idx < state.activeScenes.Size() && state.activeSceneIds.FindIndex(idx) != InvalidIndex);
    return state.activeScenes[idx];
}

//------------------------------------------------------------------------------
/**
*/
void RenderDebug()
{
}

//------------------------------------------------------------------------------
/**
*/
void
LoadMaterialTable()
{
    const IO::URI materialtable("phys:physicsmaterials.pmat");
    Util::String materialsString;
    
    if (IO::IoServer::Instance()->ReadFile(materialtable, materialsString))
    {
        Physics::MaterialsT materials;
        Flat::FlatbufferInterface::DeserializeFlatbuffer<Physics::Materials>(materials, (uint8_t*)materialsString.AsCharPtr());
        for (auto const& material : materials.entries)
        {
            CreateMaterial(material->name, material->staticFriction, material->dynamicFriction, material->restitution, material->density);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Physics::Material&
GetMaterial(IndexT idx)
{
    n_assert2(idx < state.materials.Size(), "unkown material");
    return state.materials[idx];
}

//------------------------------------------------------------------------------
/**
*/
void
SetOnSleepCallback(Util::Delegate<void(ActorId* id, SizeT num)> const& callback)
{
    state.onSleepCallback = callback;
}

//------------------------------------------------------------------------------
/**
*/
void
SetOnWakeCallback(Util::Delegate<void(ActorId* id, SizeT num)> const& callback)
{
    state.onWakeCallback = callback;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
CreateMaterial(Util::StringAtom name, float staticFriction, float dynamicFriction, float restitution, float density)
{
    n_assert(state.physics);
    PxMaterial* newMat = state.physics->createMaterial(staticFriction, dynamicFriction, restitution);
    state.materials.Append(Material());
    IndexT newIdx = state.materials.Size() - 1;
    Material & mat = state.materials[newIdx];
    mat.material = newMat;
    mat.density = density;
    mat.name = name;
    mat.serialId = name.StringHashCode();
    state.materialNameTable.Add(name, newIdx);
    return newIdx;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
LookupMaterial(Util::StringAtom name)
{
    n_assert(state.materialNameTable.Contains(name));
    return state.materialNameTable[name];
}

//------------------------------------------------------------------------------
/**
*/
SizeT
GetNrMaterials()
{
    return state.materials.Size();
}


//------------------------------------------------------------------------------
/**
*/
void
Update(Timing::Time delta)
{
    state.Update(delta);    
}

//------------------------------------------------------------------------------
/**
*/
ActorId
CreateActorInstance(Physics::ActorResourceId id, Math::mat4 const & trans, bool dynamic, uint64_t userData, IndexT scene)
{
    return Physics::actorPool->CreateActorInstance(id, trans, dynamic, userData, scene);
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyActorInstance(Physics::ActorId id)
{
    Physics::actorPool->DiscardActorInstance(id);
}

}
