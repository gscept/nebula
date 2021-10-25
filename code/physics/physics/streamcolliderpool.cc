//------------------------------------------------------------------------------
//  streamcolliderpool.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/streamcolliderpool.h"
#include "physics/physxstate.h"
#include "physics/utils.h"
#include "resources/resourceserver.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "io/uri.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flatbuffers/flatbuffers.h"
#include "flat/physics/material.h"
#include "flat/physics/collider.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/primitivegroup.h"

__ImplementClass(Physics::StreamColliderPool, 'PCRP', Resources::ResourceStreamCache);

using namespace physx;

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
Ptr<Legacy::Nvx2StreamReader> 
OpenNvx2(Util::StringAtom res)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(IO::URI(res.AsString()));
    Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
    nvx2Reader->SetStream(stream);
    nvx2Reader->SetUsage(CoreGraphics::GpuBufferTypes::UsageImmutable);
    nvx2Reader->SetAccess(CoreGraphics::GpuBufferTypes::AccessNone);
    nvx2Reader->SetRawMode(true);

    if (nvx2Reader->Open(nullptr))
    {
        return nvx2Reader;
    }
    return nullptr;
}

StreamColliderPool * colliderPool = nullptr;

//------------------------------------------------------------------------------
/**
*/
StreamColliderPool::StreamColliderPool()
{
    this->streamerThreadName = "Collider Pool Streamer Thread";
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
    ResourceStreamCache::Setup();
    this->placeholderResourceName = "phys:system/box.collider";
    this->failResourceName = "phys:system/box.collider";
}


//------------------------------------------------------------------------------
/**
*/
physx::PxGeometryHolder &
StreamColliderPool::GetGeometry(ColliderId id)
{
    this->allocator.EnterGet();
    ColliderInfo & info = this->allocator.Get<0>(id.resourceId);
    this->allocator.LeaveGet();
    return info.geometry;
}

//------------------------------------------------------------------------------
/**
*/
static physx::PxGeometryHolder
CreateMeshFromResource(MeshTopology type, Util::StringAtom resource, int primGroup)
{
    physx::PxGeometryHolder holder;

    Ptr<Legacy::Nvx2StreamReader> nvx = OpenNvx2(resource);
    
    if (nvx.isvalid())
    {        

        switch (type)
        {
            // FIXME, cooking doesnt seem to like our meshes,
            // always create convex hull, even if already convex
            case MeshTopology_Convex:
            case MeshTopology_ConvexHull:
            {
                const CoreGraphics::PrimitiveGroup& group = nvx->GetPrimitiveGroups()[primGroup];

                PxConvexMeshDesc convexDesc;
                convexDesc.points.count = nvx->GetNumVertices();
                convexDesc.points.stride = nvx->GetVertexWidth() * sizeof(float);
                convexDesc.points.data = nvx->GetVertexData();            
                convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
            
                PxDefaultMemoryOutputStream buf;
                PxConvexMeshCookingResult::Enum result;
                state.cooking->cookConvexMesh(convexDesc, buf, &result);
                PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
                holder = PxConvexMeshGeometry(state.physics->createConvexMesh(input));
            }
            break;        
            case MeshTopology_Triangles:
            {
                PxTolerancesScale scale;
                PxCookingParams params(scale);
                // disable mesh cleaning - perform mesh validation on development configurations
                //params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
                // disable edge precompute, edges are set for each triangle, slows contact generation
                params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

                state.cooking->setParams(params);

                const CoreGraphics::PrimitiveGroup& group = nvx->GetPrimitiveGroups()[primGroup];

                PxTriangleMeshDesc meshDesc;
                meshDesc.points.count = nvx->GetNumVertices();
                meshDesc.points.stride = nvx->GetVertexWidth() * sizeof(float);
                meshDesc.points.data = nvx->GetVertexData();

                meshDesc.triangles.count = group.GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList);
                meshDesc.triangles.stride = 3 * sizeof(unsigned int);
                meshDesc.triangles.data = (void*)&(nvx->GetIndexData()[group.GetBaseIndex()]);            
    #if NEBULA_DEBUG
                state.cooking->validateTriangleMesh(meshDesc);
    #endif
                PxTriangleMesh* aTriangleMesh = state.cooking->createTriangleMesh(meshDesc, state.physics->getPhysicsInsertionCallback());
                holder = PxTriangleMeshGeometry(aTriangleMesh);
            }
            break;
        }
        nvx->Close();
    }
    return holder;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceCache::LoadStatus
StreamColliderPool::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());
    n_assert(this->GetState(res) == Resources::Resource::Pending);

    /// during the load-phase, we can safetly get the structs
    this->EnterGet();
    ColliderInfo& colliderInfo = this->allocator.Get<0>(res.resourceId);
    this->LeaveGet();
    PhysicsResource::ColliderT collider;
    Flat::FlatbufferInterface::DeserializeFlatbuffer<PhysicsResource::Collider>(collider, (uint8_t*)stream->Map());

    colliderInfo.type = collider.type;

    switch (colliderInfo.type)
    {
        case ColliderType_Sphere:
        {
            float radius = collider.data.AsSphereCollider()->radius;
            colliderInfo.geometry = PxSphereGeometry(radius);
        }
        break;
        case ColliderType_Cube:
        {
            Math::vector extents = flatbuffers::UnPack(collider.data.AsBoxCollider()->box->extents());
            colliderInfo.geometry = PxBoxGeometry(Neb2PxVec(extents));
        }
        break;
        case ColliderType_Plane:
        {
            // plane is defined via transform of the actor
            colliderInfo.geometry = PxPlaneGeometry();
        }
        break;
        case ColliderType_Capsule:
        {
            auto capsule = collider.data.AsCapsuleCollider();
            float radius = capsule->radius;
            float halfHeight = capsule->halfheight;
            colliderInfo.geometry = PxCapsuleGeometry(radius, halfHeight);
        }
        break;
        case ColliderType_Mesh:
        {
            auto mesh = collider.data.AsMeshCollider();
            MeshTopology type = mesh->type;
            int primgroup = mesh->primGroup;

            Util::StringAtom resource = mesh->file;
            colliderInfo.geometry = CreateMeshFromResource(type, resource, primgroup);
        }
        break;
        default:
            n_assert("unknown collider type");
    }
    stream->Close();
    return Resources::ResourceCache::Success;
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamColliderPool::Unload(const Resources::ResourceId id)
{
    this->EnterGet();
    ColliderInfo &colliderInfo = this->allocator.Get<0>(id.resourceId);
    this->LeaveGet();

    if (colliderInfo.type == ColliderType_Mesh)
    {
        auto pxType = colliderInfo.geometry.getType();
        switch (pxType)
        {
        case physx::PxGeometryType::eCONVEXMESH:
            colliderInfo.geometry.convexMesh().convexMesh->release();
            break;
        case physx::PxGeometryType::eTRIANGLEMESH:
            colliderInfo.geometry.triangleMesh().triangleMesh->release();
            break;
        case physx::PxGeometryType::eHEIGHTFIELD:
            colliderInfo.geometry.heightField().heightField->release();
            break;
        default:
            break;
        }
    }

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

} // namespace Physics
