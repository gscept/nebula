#include "foundation/stdneb.h"
#include "physics/streamcolliderpool.h"
#include "physics/physxstate.h"
#include "physics/utils.h"
#include "resources/resourcemanager.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "io/uri.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/primitivegroup.h"

__ImplementClass(Physics::StreamColliderPool, 'PCRP', Resources::ResourceStreamPool);

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

static physx::PxGeometryHolder
CreateMeshFromResource(MeshTopologyType type, Util::StringAtom resource, int primGroup)
{
    physx::PxGeometryHolder holder;

    Ptr<Legacy::Nvx2StreamReader> nvx = OpenNvx2(resource);
    
    if (nvx.isvalid())
    {        

        switch (type)
        {
            // FIXME, cooking doesnt seem to like our meshes,
            // always create convex hull, even if already convex
            case Convex:
            case ConvexHull:
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
            case Triangles:
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
Resources::ResourcePool::LoadStatus
StreamColliderPool::LoadFromStream(const Resources::ResourceId res, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());    
    n_assert(this->GetState(res) == Resources::Resource::Pending);    

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
                    MeshTopologyType type = (MeshTopologyType)reader->GetInt("meshType");
                    int primgroup = reader->GetInt("primGroup");

                    Util::StringAtom resource = reader->GetStringAtom("file");
                    colliderInfo.geometry = CreateMeshFromResource(type, resource, primgroup);
                }
                break;
                default:
                    n_assert("unknown collider type");
            }            
            return Resources::ResourcePool::Success;
        }        
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