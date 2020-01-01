#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for physics actors

    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreampool.h"
#include "util/stack.h"
#include "physics/actorcontext.h"
#include "physics/shapes.h"
#include "physicsinterface.h"

namespace Physics
{

enum PhysicsIdType
{
    ActorIdType,
    ColliderIdType,
    MeshIdType,
};

struct ActorInfo
{
    Util::Array<physx::PxShape*> shapes;
    Util::Array<ColliderId> colliders;
    SizeT instanceCount;
    CollisionFeedbackFlag feedbackFlag;
    uint16_t collisionGroup;
    float density;    
};

    
class StreamActorPool : public Resources::ResourceStreamPool
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
    ActorId CreateActorInstance(ActorResourceId id, Math::matrix44 const & trans, bool dynamic, IndexT scene = 0);
    /// destroys an actor, if the actor was created from a resource it also
    /// reduces use count of resource
    void DiscardActorInstance(ActorId id);
      

private:
    

    /// perform actual load, override in subclass
    LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate);
    /// unload resource
    void Unload(const Resources::ResourceId id);


    Ids::IdAllocatorSafe<ActorInfo> allocator;
    __ImplementResourceAllocatorTypedSafe(allocator, ActorIdType);

};

extern StreamActorPool *actorPool;

} // namespace Physics