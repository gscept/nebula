#include "foundation/stdneb.h"
#include "physics/streamactorpool.h"
#include "physics/streamcolliderpool.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/utils.h"
#include "resources/resourcemanager.h"
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
    // empty
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
    this->placeholderResourceName = "phy:system/box.np";
    this->errorResourceName = "phy:system/box.np";
}

//------------------------------------------------------------------------------
/**
*/
ActorId 
StreamActorPool::CreateActorInstance(ActorResourceId id, Math::matrix44 const & trans, bool dynamic, IndexT scene)
{
    this->allocator.EnterGet();
    ActorInfo& info = this->allocator.Get<0>(id.allocId);
    this->allocator.LeaveGet();

    physx::PxRigidActor * newActor = state.CreateActor(dynamic, trans);
    
    for (IndexT i = 0; i < info.shapes.Size(); i++)
    {        
        newActor->attachShape(*info.shapes[i]);        
    }
    if (dynamic)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*static_cast<physx::PxRigidDynamic*>(newActor), info.density);
    }
    GetScene(scene).scene->addActor(*newActor);
    
    return ActorContext::AllocateActorId(newActor);
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus 
StreamActorPool::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());    
    n_assert(this->GetState(res) == Resources::Resource::Pending);
    
    /// during the load-phase, we can safetly get the structs
    this->EnterGet();    
    ActorInfo &actorInfo = this->allocator.Get<0>(res.allocId);       
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
                Math::matrix44 trans;
                reader->Get(trans, "transform");
                ColliderId colliderid;
                if (collider.IsValid())
                {
                    colliderid = Resources::CreateResource(collider, tag, nullptr, nullptr, true);
                    if (colliderPool->GetState(colliderid) == Resources::Resource::Failed)
                    {
                        return Resources::ResourcePool::Failed;
                    }
                }
                else
                {
                    return Resources::ResourcePool::Failed;
                }
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
}

}