#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for physics actors

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourceloader.h"
#include "util/stack.h"
#include "physics/actorcontext.h"
#include "physicsinterface.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/physics/material.h"
#include "flat/physics/constraints.h"
#include "flat/physics/actor.h"
#include "ids/idallocator.h"

namespace Physics
{

enum PhysicsIdType
{
    ActorIdType,
    MeshIdType,
};

struct BodyInfo
{
    Util::Array<physx::PxShape*> shapes;
    Util::Array<Util::String> colliders;
    Util::Array<float> densities;
#ifdef NEBULA_DEBUG
    Util::Array<Util::String> shapeDebugNames;
#endif
    uint16_t collisionGroup;
    CollisionFeedback feedbackFlag;
};

struct ActorInfo
{
    BodyInfo body; 
    Math::transform transform;
    Util::StringAtom name;
    SizeT instanceCount;
};

struct ConstraintInfo
{
    Physics::ConstraintType type;
    SizeT InstanceCount;
};

struct AggregateInfo
{
    Util::Array<ActorResourceId> bodies;
    Util::Array<ConstraintResourceId> constraints;
    SizeT instanceCount;
};
    
class StreamActorPool : public Resources::ResourceLoader
{
    __DeclareClass(StreamActorPool);
public:
    /// constructor
    StreamActorPool();
    /// destructor
    virtual ~StreamActorPool();

    /// setup resource loader, initiates the placeholder and error resources if valid
    void Setup() override;

    ///
    ActorId CreateActorInstance(ActorResourceId id, Math::transform const & trans, Physics::ActorType type, uint64_t userData, IndexT scene = 0);
    ActorId CreateActorInstance(PhysicsResourceId id, Math::transform const& trans, Physics::ActorType type, uint64_t userData, IndexT scene = 0);
    /// destroys an actor, if the actor was created from a resource it also
    /// reduces use count of resource
    void DiscardActorInstance(ActorId id);

    AggregateId CreateAggregate(PhysicsResourceId id, Math::transform const& trans, Physics::ActorType type, uint64_t userData, IndexT scene = 0);
    void DiscardAggregateInstance(AggregateId id);

    PhysicsResource::PhysicsResourceUnion GetResourceType(PhysicsResourceId id);
      

private:
    

    /// perform actual load, override in subclass
    ResourceLoader::ResourceInitOutput InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream) override;
    /// unload resource
    void Unload(const Resources::ResourceId id) override;

    enum
    {
        Info
    };
    Ids::IdAllocatorSafe<0xFFFF, ActorInfo> actorAllocator;
    Ids::IdAllocatorSafe<0xFFFF, ConstraintInfo> constraintAllocator;
    Ids::IdAllocatorSafe<0xFFFF, AggregateInfo> aggregateAllocator;
    Ids::IdAllocatorSafe<0xFFFF, PhysicsResource::PhysicsResourceUnion, Ids::Id32> resourceAllocator;
};

extern StreamActorPool *actorPool;

} // namespace Physics
