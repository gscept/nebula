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
#include "flat/physics/material.h"
#include "ids/idallocator.h"

namespace Physics
{

enum PhysicsIdType
{
    ActorIdType,
    MeshIdType,
};

struct ActorInfo
{
    Util::Array<physx::PxShape*> shapes;
    Util::Array<float> densities;
    Util::Array<Util::String> colliders;
    SizeT instanceCount;
    CollisionFeedback feedbackFlag;
    uint16_t collisionGroup;
#ifdef NEBULA_DEBUG
    Util::Array<Util::String> shapeDebugNames;
#endif
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
    void Setup();

    ///
    ActorId CreateActorInstance(ActorResourceId id, Math::mat4 const & trans, ActorType type, uint64_t userData, IndexT scene = 0);
    /// destroys an actor, if the actor was created from a resource it also
    /// reduces use count of resource
    void DiscardActorInstance(ActorId id);
      

private:
    

    /// perform actual load, override in subclass
    Resources::ResourceUnknownId InitializeResource(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate) override;
    /// unload resource
    void Unload(const Resources::ResourceId id);

    enum
    {
        Actor_Info
    };
    Ids::IdAllocatorSafe<0xFFF, ActorInfo> allocator;
};

extern StreamActorPool *actorPool;

} // namespace Physics
