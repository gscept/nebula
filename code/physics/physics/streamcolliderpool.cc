#include "foundation/stdneb.h"
#include "physics/streamcolliderpool.h"
#include "physics/physxstate.h"
#include "physics/utils.h"
#include "resources/resourcemanager.h"
#include "io/jsonreader.h"

__ImplementClass(Physics::StreamColliderPool, 'PCRP', Resources::ResourceStreamPool);

using namespace physx;

namespace Physics
{

StreamColliderPool * colliderPool = nullptr;

//------------------------------------------------------------------------------
/**
*/
StreamColliderPool::StreamColliderPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamColliderPool::~StreamColliderPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
StreamColliderPool::Setup()
{
    ResourceStreamPool::Setup();
    this->placeholderResourceName = "phy:system/box.npc";
    this->errorResourceName = "phy:system/box.npc";
}


//------------------------------------------------------------------------------
/**
*/
physx::PxGeometryHolder &
StreamColliderPool::GetGeometry(ColliderId id)
{
    this->allocator.EnterGet();
    ColliderInfo & info = this->allocator.Get<0>(id.allocId);
    this->allocator.LeaveGet();
    return info.geometry;
}


//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
StreamColliderPool::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());
    n_assert(this->GetState(res) == Resources::Resource::Pending);

    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

    /// during the load-phase, we can safetly get the structs
    this->EnterGet();
    ColliderInfo &colliderInfo = this->allocator.Get<0>(res.allocId);
    this->LeaveGet();
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        if (reader->SetToNode("/collider"))
        {
            colliderInfo.type = (ColliderType) reader->GetInt("type");
            
            switch (colliderInfo.type)
            {
                case ColliderSphere:
                {
                    float radius = reader->GetFloat("radius");
                    colliderInfo.geometry = PxSphereGeometry(radius);                    
                }
                break;
                case ColliderCube:
                {
                    Math::vector extends;
                    reader->Get(extends, "extends");
                    colliderInfo.geometry = PxBoxGeometry(Neb2PxVec(extends));
                }
                break;
                case ColliderPlane:
                {
                    // plane is defined via transform of the actor
                    colliderInfo.geometry = PxPlaneGeometry();
                }
                break;
                case ColliderCapsule:
                {
                    float radius = reader->GetFloat("radius");
                    float halfHeight = reader->GetFloat("halfHeight");
                    colliderInfo.geometry = PxCapsuleGeometry(radius, halfHeight);
                }
                break;
                case ColliderMesh:
                {

                }
                break;
                default:
                    n_assert("unknown collider type");
            }
            reader->Close();
            return Resources::ResourcePool::Success;
        }
        reader->Close();
    }
    return Resources::ResourcePool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamColliderPool::Unload(const Resources::ResourceId id)
{
}
}