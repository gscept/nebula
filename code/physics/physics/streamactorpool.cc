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
#include "io/streamreader.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/physics/material.h"
#include "flat/physics/actor.h"
#include "coregraphics/nvx3fileformatstructs.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/load/glimltypes.h"

__ImplementClass(Physics::StreamActorPool, 'PSAP', Resources::ResourceLoader);

using namespace physx;

namespace Physics
{


static PxShape* GetScaledShape(PxShape* shape, Math::vec3 const& scale);

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
    this->placeholderResourceName = "sysphys:box.actor";
    this->failResourceName = "sysphys:box.actor";
}

//------------------------------------------------------------------------------
/**
*/
ActorId
StreamActorPool::CreateActorInstance(ActorResourceId id, Math::mat4 const& trans, ActorType type, uint64_t userData, IndexT scene)
{
    __LockName(&this->allocator, lock, id.resourceId);
    ActorInfo& info = this->allocator.Get<0>(id.resourceId);

    Math::vec3 outScale; Math::quat outRotation; Math::vec3 outTranslation;
    Math::decompose(trans, outScale, outRotation, outTranslation);

    bool isScaled = Math::nearequal(outScale, Math::_plus1, 0.001f);
    
    physx::PxRigidActor * newActor = state.CreateActor(type, outTranslation, outRotation);
    info.instanceCount++;
    for (IndexT i = 0; i < info.shapes.Size(); i++)
    {
        PxShape* newShape = info.shapes[i];
        if (isScaled)
        {
            newShape = GetScaledShape(newShape, outScale);
        }
        newActor->attachShape(*newShape);
    }
    if(type == ActorType::Dynamic)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*static_cast<physx::PxRigidDynamic*>(newActor), info.densities.Begin(), info.densities.Size());
    }    
        
    GetScene(scene).scene->addActor(*newActor);
    
    ActorId newId = ActorContext::AllocateActorId(newActor, id);
    Actor& actor = ActorContext::GetActor(newId);
    actor.userData = userData;

#if NEBULA_DEBUG
    actor.debugName = Util::String::Sprintf("%s %d", this->names[id.resourceId].AsString().AsCharPtr(), newId);
    newActor->setName(actor.debugName.AsCharPtr());
#endif

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
        __LockName(&this->allocator, lock, actor.res.resourceId);
        ActorInfo& info = this->allocator.Get<0>(actor.res.resourceId);
        info.instanceCount--;
    }
    ActorContext::DiscardActor(id);
}

//------------------------------------------------------------------------------
/**
*/
static physx::PxGeometryHolder
CreateMeshFromResource(MeshTopology type, Util::StringAtom resource, int primGroup)
{
    physx::PxGeometryHolder holder;

    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(IO::URI(resource.AsString()));
    stream->SetAccessMode(IO::Stream::ReadAccess);
    Ptr<IO::StreamReader> nvx3Reader = IO::StreamReader::Create();

    nvx3Reader->SetStream(stream);

    if (!nvx3Reader->Open())
    {
        return holder;
    }

    n_assert(stream->CanBeMapped());

    void* mapPtr = nullptr;
    n_assert(nullptr == mapPtr);
    // map the stream to memory
    mapPtr = stream->MemoryMap();
    n_assert(nullptr != mapPtr);

    auto header = (CoreGraphics::Nvx3Header*)mapPtr;

    n_assert(header != nullptr);

    if (header->magic != NEBULA_NVX_MAGICNUMBER)
    {
        // not a nvx3 file, break hard
        n_error("MeshLoader: '%s' is not a nvx file!", stream->GetURI().AsString().AsCharPtr());
    }

    n_assert(header->numMeshes > 0);

    CoreGraphics::Nvx3Elements elements;
    CoreGraphics::Nvx3::FillNvx3Elements(header, elements);

    const CoreGraphics::Nvx3Group& group = elements.groups[primGroup];

    const uint vertexStride = sizeof(CoreGraphics::BaseVertex);
    const ubyte* groupVertexBase = elements.vertexData + elements.ranges[0].baseVertexByteOffset;

    const uint vertexCount = (elements.ranges[0].attributesVertexByteOffset - elements.ranges[0].baseVertexByteOffset) / vertexStride;
    
    switch (type)
    {
        // FIXME, cooking doesnt seem to like our meshes,
        // always create convex hull, even if already convex
        case MeshTopology_Convex:
        case MeshTopology_ConvexHull:
        {
            PxConvexMeshDesc convexDesc;
            convexDesc.points.count = vertexCount;
            convexDesc.points.stride = vertexStride;
            convexDesc.points.data = groupVertexBase;
            convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

            PxTolerancesScale scale;
            PxCookingParams params(scale);
            PxDefaultMemoryOutputStream buf;
            PxConvexMeshCookingResult::Enum result;
            PxCookConvexMesh(params, convexDesc, buf, &result);
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

            PxTriangleMeshDesc meshDesc;
            meshDesc.points.count = vertexCount;
            meshDesc.points.stride = vertexStride;
            meshDesc.points.data = groupVertexBase;

            meshDesc.triangles.count = group.numIndices / 3;
            if (elements.ranges[0].indexType == CoreGraphics::IndexType::Index16)
            {
                meshDesc.flags |= PxMeshFlag::e16_BIT_INDICES;
            }
            meshDesc.triangles.stride = CoreGraphics::IndexType::SizeOf(elements.ranges[0].indexType) * 3 ;
            meshDesc.triangles.data = elements.indexData + elements.ranges[0].indexByteOffset;
#if NEBULA_DEBUG
            PxValidateTriangleMesh(params, meshDesc);
#endif
            PxTriangleMesh* aTriangleMesh = PxCreateTriangleMesh(params, meshDesc, state.physics->getPhysicsInsertionCallback());
            holder = PxTriangleMeshGeometry(aTriangleMesh);
        }
        break;
    }
    nvx3Reader->Close();
    return holder;
}


static PxShape* CreateColliderShape(physx::PxGeometryHolder geometry, physx::PxMaterial* material, physx::PxTransform const& trans, const Util::String& colliderName, bool exclusive)
{
    physx::PxShape* newShape = state.physics->createShape(geometry.any(), *material, exclusive);
    n_assert(newShape != nullptr);
#ifdef NEBULA_DEBUG
    newShape->setName(colliderName.AsCharPtr());
#endif
    newShape->setLocalPose(trans);
    return newShape;
};

//------------------------------------------------------------------------------
/**
*/
static void AddCollider(physx::PxGeometryHolder geometry, IndexT material, const Math::mat4& trans, const char* name, const Util::String& colliderName, ActorInfo& actorInfo, const Util::StringAtom& tag, Ids::Id32 entry)
{
    const Physics::Material& mat = GetMaterial(material);
    actorInfo.colliders.Append(colliderName);
    Util::String shapeDebugName;
#ifdef NEBULA_DEBUG
    shapeDebugName = Util::String::Sprintf("%s %s %s %d", name, colliderName.AsCharPtr(), tag.Value(), entry);
    actorInfo.shapeDebugNames.Append(shapeDebugName);
#endif
    PxShape* newShape = CreateColliderShape(geometry, mat.material, Neb2PxTrans(trans), shapeDebugName, false);
    actorInfo.shapes.Append(newShape);
    actorInfo.densities.Append(GetMaterial(material).density);
}

//------------------------------------------------------------------------------
/**
*/
static void
AddMeshColliders(PhysicsResource::MeshColliderT* colliderNode, Math::mat4 const& nodeTransform, Util::String nodeName, IndexT materialIdx, const Util::StringAtom& tag, Ids::Id32 entry, ActorInfo& targetActor)
{
    MeshTopology type = colliderNode->type;
    int primGroup = colliderNode->prim_group;

    Util::StringAtom resource = colliderNode->file;
    if (type != MeshTopology_ApproxSkin)
    {
        physx::PxGeometryHolder geometry = CreateMeshFromResource(type, resource, primGroup);
        AddCollider(geometry, materialIdx, nodeTransform, resource.Value(), nodeName, targetActor, tag, entry);
        return;
    }
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(IO::URI(resource.AsString()));
    stream->SetAccessMode(IO::Stream::ReadAccess);
    Ptr<IO::StreamReader> nvx3Reader = IO::StreamReader::Create();

    nvx3Reader->SetStream(stream);

    if (!nvx3Reader->Open())
    {
        return;
    }

    n_assert(stream->CanBeMapped());

    void* mapPtr = nullptr;
    n_assert(nullptr == mapPtr);
    // map the stream to memory
    mapPtr = stream->MemoryMap();
    n_assert(nullptr != mapPtr);

    auto header = (CoreGraphics::Nvx3Header*)mapPtr;

    n_assert(header != nullptr);

    if (header->magic != NEBULA_NVX_MAGICNUMBER)
    {
        // not a nvx3 file, break hard
        n_error("MeshLoader: '%s' is not a nvx file!", stream->GetURI().AsString().AsCharPtr());
    }

    n_assert(header->numMeshes > 0);

    CoreGraphics::Nvx3Elements elements;
    CoreGraphics::Nvx3::FillNvx3Elements(header, elements);

    const CoreGraphics::Nvx3Group& group = elements.groups[primGroup];

    const uint vertexStride = sizeof(CoreGraphics::BaseVertex);
    const ubyte* groupVertexBase = elements.vertexData + elements.ranges[0].baseVertexByteOffset;
    const ubyte* attributeVertexBase = elements.vertexData + elements.ranges[0].attributesVertexByteOffset;

    const uint vertexCount = (elements.ranges[0].attributesVertexByteOffset - elements.ranges[0].baseVertexByteOffset) / vertexStride;
    n_assert(elements.ranges[0].layout == CoreGraphics::VertexLayoutType::Skin);

    Util::Array<PxVec3> targetVertexBuffer(vertexCount, 10);
    // this is super inefficient, but also pointless, so who cares :D
    CoreGraphics::SkinVertex* skinBuffer = (CoreGraphics::SkinVertex*)attributeVertexBase;
    CoreGraphics::BaseVertex* vertexBuffer = (CoreGraphics::BaseVertex*)groupVertexBase;
    ubyte maxIndex = 1;
    ubyte currBone = 0;
    while (currBone < maxIndex)
    {
        targetVertexBuffer.Reset();
        for (int i = 0; i < vertexCount; ++i)
        {
            CoreGraphics::SkinVertex const& vertex = skinBuffer[i];
            CoreGraphics::BaseVertex const& pos = vertexBuffer[i];

            if (vertex.skinWeights[0] > 0.0f)
            {
                if (currBone == vertex.skinIndices.x)
                {
                    targetVertexBuffer.Append({ pos.position[0], pos.position[1], pos.position[2] });
                }
                else
                {
                    maxIndex = Math::max(vertex.skinIndices.x, maxIndex);
                }
            }
        }
        if (targetVertexBuffer.Size() > 0)
        {
            PxConvexMeshDesc convexDesc;
            convexDesc.points.count = targetVertexBuffer.Size();
            convexDesc.points.stride = sizeof (PxVec3);
            convexDesc.points.data = &targetVertexBuffer[0];
            convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

            PxDefaultMemoryOutputStream buf;
            PxConvexMeshCookingResult::Enum result;
            PxTolerancesScale scale;
            PxCookingParams params(scale);
            PxCookConvexMesh(params, convexDesc, buf, &result);
            if (result == PxConvexMeshCookingResult::eSUCCESS)
            {
                PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
                physx::PxGeometryHolder holder = PxConvexMeshGeometry(state.physics->createConvexMesh(input));
                Util::String BoneName = Util::String::Sprintf("%s_%d", nodeName.AsCharPtr(), currBone);
                AddCollider(holder, materialIdx, nodeTransform, resource.Value(), BoneName, targetActor, tag, entry);
            }
            else
            {
                n_printf("Convex mesh failed: %d\n", result);
            }
        }
        currBone++;
    }
    nvx3Reader->Close();
}

//------------------------------------------------------------------------------
/**
*/
static void
AddHeightField(PhysicsResource::HeightFieldColliderT* colliderNode, Math::mat4 const& nodeTransform, Util::String nodeName, IndexT materialIdx, const Util::StringAtom& tag, Ids::Id32 entry, ActorInfo& targetActor)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(IO::URI(colliderNode->file));
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (stream->Open())
    {
        void* heightData = stream->MemoryMap();
        uint heightSize = stream->GetSize();
        n_assert(heightData != nullptr);

        gliml::context ctx;
        if (ctx.load_dds(heightData, heightSize))
        {
            int depth = ctx.image_depth(0, 0);
            int width = ctx.image_width(0, 0);
            int height = ctx.image_height(0, 0);

            CoreGraphics::PixelFormat::Code format = CoreGraphics::Gliml::ToPixelFormat(ctx);
            n_assert(format == CoreGraphics::PixelFormat::R16);

            PxHeightFieldSample* heightSamples = (PxHeightFieldSample*)Memory::Alloc(Memory::PhysicsHeap, width * height * sizeof(PxHeightFieldSample));
            n_assert(heightSamples != nullptr);
            Memory::Clear(heightSamples, width * height * sizeof(PxHeightFieldSample));

            uint16_t* heightBuffer = (uint16_t*)ctx.image_data(0, 0);

            // need to flip to be in same orientation with rendering
            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    heightSamples[i * width + j].height = heightBuffer[j * width + i];
                }
            }

            PxHeightFieldDesc hfDesc;
            hfDesc.format = PxHeightFieldFormat::eS16_TM;
            hfDesc.nbColumns = width;
            hfDesc.nbRows = height;
            hfDesc.samples.data = heightSamples;
            hfDesc.samples.stride = sizeof(PxHeightFieldSample);

            PxHeightField* physxHeightField = PxCreateHeightField(hfDesc);

            float sh = colliderNode->height_range / 65536.0f;
            float sx = colliderNode->target_size_x / (float)width;
            float sy = colliderNode->target_size_y / (float)height;
            PxHeightFieldGeometry hfGeom(physxHeightField, PxMeshGeometryFlags(), sh, sx, sy);
            Math::mat4 offsetTransform = Math::mat4::identity;
            offsetTransform.position = Math::vec4(-0.5f * colliderNode->target_size_x, 0.0f, -0.5f * colliderNode->target_size_y, 1.0f);
            physx::PxGeometryHolder holder = hfGeom;
            AddCollider(holder, materialIdx, offsetTransform, colliderNode->file.AsCharPtr(), nodeName, targetActor, tag, entry);
            Memory::Free(Memory::HeapType::PhysicsHeap, heightSamples);
        }
        stream->MemoryUnmap();
        stream->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
StreamActorPool::InitializeResource(const Ids::Id32 entry, const Util::StringAtom & tag, const Ptr<IO::Stream>& stream, bool immediate)
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
        
        Math::mat4 trans = shape->transform;

        switch (collider->type)
        {
            case ColliderType_Sphere:
            {
                
                float radius = collider->data.AsSphereCollider()->radius;
                physx::PxGeometryHolder geometry = PxSphereGeometry(radius);
                AddCollider(geometry, material, shape->transform, "Sphere", collider->name, actorInfo, tag, entry);
            }
            break;
            case ColliderType_Cube:
            {
                Math::vector extents = collider->data.AsBoxCollider()->extents;
                physx::PxGeometryHolder geometry = PxBoxGeometry(Neb2PxVec(extents));
                AddCollider(geometry, material, shape->transform, "Cube", collider->name, actorInfo, tag, entry);
            }
            break;
            case ColliderType_Plane:
            {
                // plane is defined via transform of the actor
                physx::PxGeometryHolder geometry = PxPlaneGeometry();
                AddCollider(geometry, material, shape->transform, "Plane", collider->name, actorInfo, tag, entry);
            }
            break;
            case ColliderType_Capsule:
            {
                auto capsule = collider->data.AsCapsuleCollider();
                float radius = capsule->radius;
                float halfHeight = capsule->halfheight;
                physx::PxGeometryHolder geometry = PxCapsuleGeometry(radius, halfHeight);
                AddCollider(geometry, material, shape->transform, "Capsule", collider->name, actorInfo, tag, entry);
            }
            break;
            case ColliderType_Mesh:
            {
                AddMeshColliders(collider->data.AsMeshCollider(), shape->transform, collider->name, material, tag, entry, actorInfo);
/*
                auto mesh = collider->data.AsMeshCollider();
                MeshTopology type = mesh->type;
                int primgroup = mesh->prim_group;

                Util::StringAtom resource = mesh->file;
                physx::PxGeometryHolder geometry = CreateMeshFromResource(type, resource, primgroup);
                AddCollider(geometry, material, shape->transform, resource.Value(), collider->name);
                */
            }
            break;
            case ColliderType_HeightField:
            {
                AddHeightField(collider->data.AsHeightFieldCollider(), shape->transform, collider->name, material, tag, entry, actorInfo);
            }
            break;
            default:
                n_error("unknown collider type");
        }        
    }

    Ids::Id32 id = allocator.Alloc();
    allocator.Set<Actor_Info>(id, actorInfo);

    ActorResourceId ret;
    ret.resourceId = id;
    ret.resourceType = Physics::ActorIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamActorPool::Unload(const Resources::ResourceId id)
{
    __LockName(&this->allocator, lock, id.resourceId);
    ActorInfo& info = this->allocator.Get<0>(id.resourceId);
    n_assert2(info.instanceCount == 0, "Actor has active Instances");
    const Util::StringAtom tag = this->GetTag(id);
    
    for (auto i : info.shapes)
    {
        i->release();
    }
    info.shapes.Clear();
    allocator.Dealloc(id.resourceId);
}


static PxShape* GetScaledShape(PxShape* shape, Math::vec3 const& inScale)
{
    n_assert(shape != nullptr);
    physx::PxTransform localTrans = shape->getLocalPose();
    const physx::PxGeometry& geometry = shape->getGeometry();
    physx::PxGeometryType::Enum type = geometry.getType();
    physx::PxVec3 scale = Neb2PxVec(inScale);

    //localTrans.p = localTrans.p.multiply(scale);
    PxGeometryHolder holder;
    Util::String debugName;
#ifdef NEBULA_DEBUG
    
#endif
    PxShape* newShape = nullptr;
    switch (type)
    {
        case physx::PxGeometryType::eSPHERE:
        {
            const float radiusScale = scale.maxElement();
            PxSphereGeometry sGeom = static_cast<const PxSphereGeometry&>(geometry);
            const float newRadius = sGeom.radius * radiusScale;
            holder = PxSphereGeometry(newRadius);
        }
        break;
        case physx::PxGeometryType::eCAPSULE:
        {   
            PxCapsuleGeometry cGeom = static_cast<const PxCapsuleGeometry&>(geometry);
            holder = PxCapsuleGeometry(cGeom.radius * scale.x, cGeom.halfHeight * scale.z);
        }
        break;
        case physx::PxGeometryType::eBOX:
        {
            PxBoxGeometry bGeom = static_cast<const PxBoxGeometry&>(geometry);
            physx::PxVec3 half = bGeom.halfExtents;
            half = half.multiply(scale);
            holder = PxBoxGeometry(half);
        }
        break;
        case physx::PxGeometryType::eCONVEXMESH:
        {
            physx::PxConvexMeshGeometry geom = static_cast<const PxConvexMeshGeometry&>(geometry);
            physx::PxMeshScale meshScale(scale, physx::PxQuat(physx::PxIDENTITY::PxIdentity));
            holder = physx::PxConvexMeshGeometry(geom.convexMesh, meshScale);
        }
        break;
        case physx::PxGeometryType::eTRIANGLEMESH:
        {
            physx::PxTriangleMeshGeometry geom = static_cast<const PxTriangleMeshGeometry&>(geometry);
            physx::PxMeshScale meshScale(scale, physx::PxQuat(physx::PxIDENTITY::PxIdentity));
            holder = physx::PxTriangleMeshGeometry(geom.triangleMesh, meshScale);
        }
        break;
        default:
            n_assert_fmt(false, "unsupported mesh type %d", type);
    }
    const uint16 mats = shape->getNbMaterials();
    physx::PxMaterial* mat[16];
    n_assert(mats < 16);
    shape->getMaterials((physx::PxMaterial **) &mat, 16, 0);
    return CreateColliderShape(holder, mat[0], localTrans, debugName, true);
}
}
