//------------------------------------------------------------------------------
//  streamactorpool.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/streamactorpool.h"
#include "physics/streamcolliderpool.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/utils.h"
#include "resources/resourceserver.h"
#include "io/jsonreader.h"

__ImplementClass(Physics::StreamActorPool, 'PSAP', Resources::ResourceStreamPool);

namespace Physics
{

StreamActorPool *actorPool = nullptr;
//------------------------------------------------------------------------------
/**
*/
StreamActorPool::StreamActorPool()
{
    this->streamerThreadName = "Physics Actor Pool Streamer Thread";
}

//------------------------------------------------------------------------------
/**
*/
StreamActorPool::~StreamActorPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamActorPool::Setup()
{
    ResourceStreamPool::Setup();
    this->placeholderResourceName = "phys:system/box.np";
    this->failResourceName = "phys:system/box.np";
}

//------------------------------------------------------------------------------
/**
*/
ActorId 
StreamActorPool::CreateActorInstance(ActorResourceId id, Math::mat4 const & trans, bool dynamic, IndexT scene)
{
    this->allocator.EnterGet();
    ActorInfo& info = this->allocator.Get<0>(id.resourceId);
    this->allocator.LeaveGet();

    physx::PxRigidActor * newActor = state.CreateActor(dynamic, trans);
    info.instanceCount++;
    for (IndexT i = 0; i < info.shapes.Size(); i++)
    {        
        newActor->attachShape(*info.shapes[i]);        
    }
    if (dynamic)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*static_cast<physx::PxRigidDynamic*>(newActor), info.density);
    }
    GetScene(scene).scene->addActor(*newActor);
    
    return ActorContext::AllocateActorId(newActor, id);
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamActorPool::DiscardActorInstance(ActorId id)
{
    n_assert(ActorContext::IsValid(id));
    Actor& actor = ActorContext::GetActor(id);
    if (actor.res != ActorResourceId::Invalid())
    {
        this->allocator.EnterGet();
        ActorInfo& info = this->allocator.Get<0>(actor.res.resourceId);
        this->allocator.LeaveGet();
        info.instanceCount--;
    }
    ActorContext::DiscardActor(id);
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus 
StreamActorPool::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());    
    n_assert(this->GetState(res) == Resources::Resource::Pending);
    
    /// during the load-phase, we can safetly get the structs
    this->EnterGet();    
    ActorInfo &actorInfo = this->allocator.Get<0>(res.resourceId);
    actorInfo.instanceCount = 0;
    this->LeaveGet();
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        if (reader->SetToNode("/actor"))
        {
            actorInfo.feedbackFlag = (Physics::CollisionFeedbackFlag)reader->GetInt("feedback");            
            reader->Get(actorInfo.density, "density");
            reader->SetToFirstChild("colliders");
            reader->SetToFirstChild();
            do
            {
                Util::String name = reader->GetString("name");
                Util::StringAtom matAtom = reader->GetStringAtom("material");                
                IndexT material = LookupMaterial(matAtom);

                Resources::ResourceName collider = name;
                
                uint16_t group;
                reader->Get(group, "group");
                Math::mat4 trans;
                reader->Get(trans, "transform");
                ColliderId colliderid;
                if (collider.IsValid())
                {
                    colliderid = Resources::CreateResource(collider, "", nullptr, nullptr, true);
                    if (colliderPool->GetState(colliderid) == Resources::Resource::Failed)
                    {
                        return Resources::ResourcePool::Failed;
                    }
                }
                else
                {
                    return Resources::ResourcePool::Failed;
                }
                actorInfo.colliders.Append(colliderid);
                physx::PxGeometryHolder & geom = colliderPool->GetGeometry(colliderid);
                physx::PxShape *newShape = state.physics->createShape(geom.any(), *state.materials[material].material);
                newShape->setLocalPose(Neb2PxTrans(trans));
                actorInfo.shapes.Append(newShape);

            } while (reader->SetToNextChild());                            
            return Resources::ResourcePool::Success;                                    
        }
    }    
    return Resources::ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamActorPool::Unload(const Resources::ResourceId id)
{
    this->EnterGet();
    ActorInfo& info = this->allocator.Get<0>(id.resourceId);
    this->LeaveGet();
    n_assert2(info.instanceCount == 0, "Actor has active Instances");
    const Util::StringAtom tag = this->GetTag(id);
    
    for (auto i : info.colliders)
    {
            colliderPool->DiscardResource(i);
    }
    for (auto s : info.shapes)
    {
        s->release();        
    }    

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

}
