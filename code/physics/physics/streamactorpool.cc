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
#include "coregraphics/indextype.h"

__ImplementClass(Physics::StreamActorPool, 'PSAP', Resources::ResourceLoader);

using namespace physx;

namespace Physics
{


static PxShape* GetShapeCopy(PxShape* shape, Math::vec3 const& scale);

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
StreamActorPool::CreateActorInstance(PhysicsResourceId id, Math::transform const& trans, Physics::ActorType type, uint64_t userData, IndexT scene)
{
    n_assert2(this->resourceAllocator.Get<0>(id.id) == PhysicsResource::PhysicsResourceUnion_BodySetup, "Trying to create actor instance from non-actor resource");
    ActorResourceId actId = this->resourceAllocator.Get<1>(id.id);
    return this->CreateActorInstance(actId, trans, type, userData, scene);
}
//------------------------------------------------------------------------------
/**
*/
ActorId
StreamActorPool::CreateActorInstance(ActorResourceId id, Math::transform const& worldTrans, Physics::ActorType type, uint64_t userData, IndexT scene)
{
    n_assert(id.resourceId != Physics::InvalidPhysicsResourceId.resourceId);
    __LockName(&this->actorAllocator, lock, id.id);
    ActorInfo& info = this->actorAllocator.Get<Info>(id.id);

    Math::transform trans = worldTrans * info.transform;
    bool isScaled = !Math::nearequal(trans.scale, Math::_plus1, 0.001f);
    

    info.instanceCount++;
    
    const auto& bi = info.body;
    physx::PxRigidActor* newActor = state.CreateActor(type, trans.position, trans.rotation);
        
    for (IndexT i = 0; i < info.body.shapes.Size(); i++)
    {
        PxShape* newShape = GetShapeCopy(info.body.shapes[i], trans.scale);
        newActor->attachShape(*newShape);
    }
    if (type == ActorType::Dynamic)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*static_cast<physx::PxRigidDynamic*>(newActor), info.body.densities.Begin(), info.body.densities.Size());
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
    if(!ActorContext::IsValid(id))
    {
        return;
    }
    Actor& actor = ActorContext::GetActor(id);
    if (actor.res != ActorResourceId::Invalid())
    {
        __LockName(&this->actorAllocator, lock, actor.res.id);
        ActorInfo& info = this->actorAllocator.Get<Info>(actor.res.id);
        info.instanceCount--;
    }
    ActorContext::DiscardActor(id);
}

//------------------------------------------------------------------------------
/**
*/
AggregateId 
StreamActorPool::CreateAggregate(PhysicsResourceId id, Math::transform const& trans, Physics::ActorType type, uint64_t userData, IndexT scene)
{
    n_assert2(this->resourceAllocator.Get<0>(id.id) == PhysicsResource::PhysicsResourceUnion_Aggregate, "Trying to create aggregate actor instance from invalid resource");
    __LockName(&this->aggregateAllocator, lock, id.id);
    AggregateResourceId aggId = this->resourceAllocator.Get<1>(id.id);
    AggregateInfo& info = this->aggregateAllocator.Get<Info>(aggId.id);
    info.instanceCount++;

    AggregateId aggInstanceId = AggregateContext::AllocateAggregateId(aggId);
    Aggregate aggregate = AggregateContext::GetAggregate(aggInstanceId);
    Util::Dictionary<Util::StringAtom, physx::PxActor*> actorDict;
    for (ActorResourceId body : info.bodies)
    {
        ActorId actorId = this->CreateActorInstance(body, trans, type, userData, scene);
        Actor& actor = ActorContext::GetActor(actorId);
        const ActorInfo & actorInfo = this->actorAllocator.Get<0>(actor.res.id);
        actorDict.Add(actorInfo.name, actor.actor);
        aggregate.actors.Append(actorId);
    }
    return aggInstanceId;
}

//------------------------------------------------------------------------------
/**
*/
PhysicsResource::PhysicsResourceUnion 
StreamActorPool::GetResourceType(PhysicsResourceId id)
{
    return this->resourceAllocator.Get<0>(id.id);
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
    CoreGraphics::Nvx3::FillNvx3Elements((char*)mapPtr, header, elements);

    const uint vertexStride = sizeof(CoreGraphics::BaseVertex);
    const ubyte* groupVertexBase = elements.vertexData + elements.ranges[0].baseVertexByteOffset;
    const uint vertexCount = (elements.ranges[0].attributesVertexByteOffset - elements.ranges[0].baseVertexByteOffset) / vertexStride;
    const CoreGraphics::Nvx3Group* group = (CoreGraphics::Nvx3Group*)((char*)mapPtr + elements.ranges[0].firstGroupOffset + sizeof(CoreGraphics::Nvx3Group) * primGroup);

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

            meshDesc.triangles.count = group->numIndices / 3;
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
static void AddCollider(physx::PxGeometryHolder geometry, IndexT material, const Math::transform& trans, const char* name, const Util::String& colliderName, BodyInfo& bodyInfo, const Util::StringAtom& tag, Ids::Id32 entry)
{
    const Physics::Material& mat = GetMaterial(material);
    bodyInfo.colliders.Append(colliderName);
    Util::String shapeDebugName;
#ifdef NEBULA_DEBUG
    shapeDebugName = Util::String::Sprintf("%s %s %s %d", name, colliderName.AsCharPtr(), tag.Value(), entry);
    bodyInfo.shapeDebugNames.Append(shapeDebugName);
#endif
    PxShape* newShape = CreateColliderShape(geometry, mat.material, Neb2PxTrans(trans), shapeDebugName, true);
    bodyInfo.shapes.Append(newShape);
    bodyInfo.densities.Append(GetMaterial(material).density);
}

//------------------------------------------------------------------------------
/**
*/
static void
AddMeshColliders(PhysicsResource::MeshColliderT* colliderNode, Math::transform const& nodeTransform, Util::String nodeName, IndexT materialIdx, const Util::StringAtom& tag, Ids::Id32 entry, BodyInfo& targetBody)
{
    MeshTopology type = colliderNode->type;
    int primGroup = colliderNode->prim_group;

    Util::StringAtom resource = colliderNode->file;
    if (type != MeshTopology_ApproxSkin)
    {
        physx::PxGeometryHolder geometry = CreateMeshFromResource(type, resource, primGroup);
        AddCollider(geometry, materialIdx, nodeTransform, resource.Value(), nodeName, targetBody, tag, entry);
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
    char* basePtr = (char*)mapPtr;
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
    CoreGraphics::Nvx3::FillNvx3Elements(basePtr, header, elements);

    const CoreGraphics::Nvx3Group* group = (CoreGraphics::Nvx3Group*)(basePtr + elements.ranges[0].firstGroupOffset + sizeof(CoreGraphics::Nvx3Group) * primGroup);

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
                AddCollider(holder, materialIdx, nodeTransform, resource.Value(), BoneName, targetBody, tag, entry);
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
AddHeightField(PhysicsResource::HeightFieldColliderT* colliderNode, Math::transform const& nodeTransform, Util::String nodeName, IndexT materialIdx, const Util::StringAtom& tag, Ids::Id32 entry, BodyInfo& targetBody)
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
            Math::transform offsetTransform;
            offsetTransform.position = Math::vec3(-0.5f * colliderNode->target_size_x, 0.0f, -0.5f * colliderNode->target_size_y);
            physx::PxGeometryHolder holder = hfGeom;
            AddCollider(holder, materialIdx, offsetTransform, colliderNode->file.AsCharPtr(), nodeName, targetBody, tag, entry);
            Memory::Free(Memory::HeapType::PhysicsHeap, heightSamples);
        }
        stream->MemoryUnmap();
        stream->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
StreamActorPool::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());    
    Resources::ResourceLoader::ResourceInitOutput ret;
    
    PhysicsResource::ActorT actor;
    Flat::FlatbufferInterface::DeserializeFlatbuffer<PhysicsResource::Actor>(actor, (uint8_t*)stream->Map());

       
    static const auto parseBody = [](const PhysicsResource::BodyT& body, const ResourceLoadJob& job)
        {
            BodyInfo bodyInfo;
            bodyInfo.feedbackFlag = body.feedback;
            for (auto const& shape : body.shapes)
            {
                auto const& collider = shape->collider;
                Util::StringAtom matAtom = shape->material;
                IndexT material = LookupMaterial(matAtom);

                switch (collider->type)
                {
                case ColliderType_Sphere:
                {

                    float radius = collider->data.AsSphereCollider()->radius;
                    physx::PxGeometryHolder geometry = PxSphereGeometry(radius);
                AddCollider(geometry, material, shape->transform, "Sphere", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Cube:
                {
                    Math::vector extents = collider->data.AsBoxCollider()->extents;
                    physx::PxGeometryHolder geometry = PxBoxGeometry(Neb2PxVec(extents));
                AddCollider(geometry, material, shape->transform, "Cube", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Plane:
                {
                    // plane is defined via transform of the actor
                    physx::PxGeometryHolder geometry = PxPlaneGeometry();
                AddCollider(geometry, material, shape->transform, "Plane", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Capsule:
                {
                    auto capsule = collider->data.AsCapsuleCollider();
                    float radius = capsule->radius;
                    float halfHeight = capsule->halfheight;
                    physx::PxGeometryHolder geometry = PxCapsuleGeometry(radius, halfHeight);
                AddCollider(geometry, material, shape->transform, "Capsule", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Mesh:
                {
                AddMeshColliders(collider->data.AsMeshCollider(), shape->transform, collider->name, material, job.tag, job.id.loaderInstanceId, bodyInfo);
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
                AddHeightField(collider->data.AsHeightFieldCollider(), shape->transform, collider->name, material, job.tag, job.id.loaderInstanceId, bodyInfo);
                }
                break;
                default:
                    n_error("unknown collider type");
                }
            }
            return bodyInfo;
        };

    Ids::Id32 resId = this->resourceAllocator.Alloc();
    this->resourceAllocator.Set<0>(resId, actor.data.type);
    switch (actor.data.type)
    {
        case PhysicsResource::PhysicsResourceUnion_BodySetup:
        {
            /// during the load-phase, we can safetly get the structs
            ActorInfo actorInfo;
            actorInfo.instanceCount = 0;

            const PhysicsResource::BodySetupT* bodySetup = actor.data.AsBodySetup();
            auto& body = actorInfo.body;
            body = parseBody(*bodySetup->body, job);
            actorInfo.name = bodySetup->name;
            actorInfo.transform = bodySetup->transform;

            Ids::Id32 id = this->actorAllocator.Alloc();
            this->actorAllocator.Set<Info>(id, actorInfo);
            this->resourceAllocator.Set<1>(resId, id);
        }
        break;
        case PhysicsResource::PhysicsResourceUnion_Aggregate:
        {
            AggregateInfo aggInfo;
            aggInfo.instanceCount = 0;
            const PhysicsResource::AggregateT* aggSetup = actor.data.AsAggregate();
            aggInfo.bodies.Reserve((SizeT)aggSetup->bodies.size());
            for (auto const& bodySetup : aggSetup->bodies)
            {
                Ids::Id32 actorId = this->actorAllocator.Alloc();
                aggInfo.bodies.Append(actorId);

                ActorInfo actorInfo;
                actorInfo.instanceCount = 0;                
                actorInfo.body = parseBody(*bodySetup->body, job);
                actorInfo.name = bodySetup->name;
                actorInfo.transform = bodySetup->transform;
                actorAllocator.Set<Info>(actorId, actorInfo);
            }
            Ids::Id32 aggId = this->aggregateAllocator.Alloc();
            this->aggregateAllocator.Set<Info>(aggId, aggInfo);
            this->resourceAllocator.Set<1>(resId, aggId);
        }
        break;
        default:
            ret.id = Physics::InvalidPhysicsResourceId;
            return ret;
    }
    ret.id = resId;
    return ret;    
}

//------------------------------------------------------------------------------
/**
*/
void
StreamActorPool::Unload(const Resources::ResourceId id)
{
    __LockName(&this->resourceAllocator, lock, id.resource);
    
    static const auto freeShapes = [&](Physics::ActorInfo& actor)
        {
            n_assert2(actor.instanceCount == 0, "Actor has active Instances");

            for (auto i : actor.body.shapes)
            {
                i->release();
            }
            actor.body.shapes.Clear();
        };
    Ids::Id32 resId = this->resourceAllocator.Get<1>(id.resource);
    switch (this->resourceAllocator.Get<0>(id.resource))
    {
        case PhysicsResource::PhysicsResourceUnion_BodySetup:
        {
            ActorInfo& info = this->actorAllocator.Get<0>(resId);
            freeShapes(info);
            actorAllocator.Dealloc(resId);
	    }
        break;
        case PhysicsResource::PhysicsResourceUnion_Aggregate:
        {
            AggregateInfo& info = this->aggregateAllocator.Get<0>(resId);
            for (ActorResourceId actorId : info.bodies)
            {
                ActorInfo& actor = this->actorAllocator.Get<0>(actorId.id);
                freeShapes(actor);
                this->actorAllocator.Dealloc(actorId.id);
            }
            this->aggregateAllocator.Dealloc(resId);
        }
        break;
    }    
    this->resourceAllocator.Dealloc(id.resource);
}


static PxShape* GetShapeCopy(PxShape* shape, Math::vec3 const& inScale)
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
        case physx::PxGeometryType::eHEIGHTFIELD:
        {
            // we never copy these
            return shape;
        }
        break;
        default:
            n_assert_fmt(false, "unsupported mesh type %d", type);
    }
    const uint16_t mats = shape->getNbMaterials();
    physx::PxMaterial* mat[16];
    n_assert(mats < 16);
    shape->getMaterials((physx::PxMaterial **) &mat, 16, 0);
    return CreateColliderShape(holder, mat[0], localTrans, debugName, true);
}
}
