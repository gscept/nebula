//------------------------------------------------------------------------------
//  streamactorpool.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "physics/streamactorpool.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/utils.h"
#include "resources/resourceserver.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/physics/material.h"
#include "flat/physics/actor.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/primitivegroup.h"

__ImplementClass(Physics::StreamActorPool, 'PSAP', Resources::ResourceLoader);

using namespace physx;

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
    ResourceLoader::Setup();
    this->placeholderResourceName = "phys:system/box.actor";
    this->failResourceName = "phys:system/box.actor";
}

//------------------------------------------------------------------------------
/**
*/
ActorId
StreamActorPool::CreateActorInstance(ActorResourceId id, Math::mat4 const& trans, bool dynamic, uint64_t userData, IndexT scene)
{
    __LockName(&this->allocator, lock, Util::ArrayAllocatorAccess::Write);
    ActorInfo& info = this->allocator.Get<0>(id.resourceId);

    physx::PxRigidActor * newActor = state.CreateActor(dynamic, trans);
    info.instanceCount++;
    for (IndexT i = 0; i < info.shapes.Size(); i++)
    {        
        newActor->attachShape(*info.shapes[i]);
    }
    if (dynamic)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*static_cast<physx::PxRigidDynamic*>(newActor), info.densities.Begin(), info.densities.Size());
    }
    
    GetScene(scene).scene->addActor(*newActor);
    
    ActorId newId = ActorContext::AllocateActorId(newActor, id);
    ActorContext::GetActor(newId).userData = userData;

    return newId;
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
        __LockName(&this->allocator, lock, Util::ArrayAllocatorAccess::Write);
        ActorInfo& info = this->allocator.Get<0>(actor.res.resourceId);
        info.instanceCount--;
    }
    ActorContext::DiscardActor(id);
}


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
Resources::ResourceUnknownId
StreamActorPool::LoadFromStream(const Ids::Id32 entry, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    n_assert(stream.isvalid());    
    
    /// during the load-phase, we can safetly get the structs
    ActorInfo actorInfo;
    actorInfo.instanceCount = 0;
    PhysicsResource::ActorT actor;
    Flat::FlatbufferInterface::DeserializeFlatbuffer<PhysicsResource::Actor>(actor, (uint8_t*)stream->Map());
    
    actorInfo.feedbackFlag = actor.feedback;

    for (auto const& shape : actor.shapes)
    {
        auto const & collider = shape->collider;
        Util::StringAtom matAtom = shape->material;
        IndexT material = LookupMaterial(matAtom);
        physx::PxGeometryHolder geometry;

        Math::mat4 trans = shape->transform;

        switch (collider->type)
        {
            case ColliderType_Sphere:
            {
                float radius = collider->data.AsSphereCollider()->radius;
                geometry = PxSphereGeometry(radius);
            }
            break;
            case ColliderType_Cube:
            {
                Math::vector extents = collider->data.AsBoxCollider()->extents;
                geometry = PxBoxGeometry(Neb2PxVec(extents));
            }
            break;
            case ColliderType_Plane:
            {
                // plane is defined via transform of the actor
                geometry = PxPlaneGeometry();
            }
            break;
            case ColliderType_Capsule:
            {
                auto capsule = collider->data.AsCapsuleCollider();
                float radius = capsule->radius;
                float halfHeight = capsule->halfheight;
                geometry = PxCapsuleGeometry(radius, halfHeight);
            }
            break;
            case ColliderType_Mesh:
            {
                auto mesh = collider->data.AsMeshCollider();
                MeshTopology type = mesh->type;
                int primgroup = mesh->prim_group;

                Util::StringAtom resource = mesh->file;
                geometry = CreateMeshFromResource(type, resource, primgroup);
            }
            break;
            default:
                n_assert("unknown collider type")
        }
        actorInfo.colliders.Append(collider->name);
        physx::PxShape* newShape = state.physics->createShape(geometry.any(), *state.materials[material].material);
        newShape->setLocalPose(Neb2PxTrans(trans));
        actorInfo.shapes.Append(newShape);
        actorInfo.densities.Append(GetMaterial(material).density);
    }

    __Lock(allocator, Util::ArrayAllocatorAccess::Write);
    Ids::Id32 id = allocator.Alloc();
    allocator.Set<Actor_Info>(id, actorInfo);

    ActorResourceId ret;
    ret.resourceId = id;
    ret.resourceType = 0;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamActorPool::Unload(const Resources::ResourceId id)
{
    __LockName(&this->allocator, lock, Util::ArrayAllocatorAccess::Write);
    ActorInfo& info = this->allocator.Get<0>(id.resourceId);
    n_assert2(info.instanceCount == 0, "Actor has active Instances");
    const Util::StringAtom tag = this->GetTag(id);
    
    for (auto i : info.shapes)
    {
        i->release();
    }
    for (auto s : info.shapes)
    {
        s->release();        
    }    
    allocator.Dealloc(id.resourceId);
}

}
