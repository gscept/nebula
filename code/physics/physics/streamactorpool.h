#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for physics actors

    (C) 2019 Individual contributors, see AUTHORS file
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
};



RESOURCE_ID_TYPE(ActorResourceId);
RESOURCE_ID_TYPE(ColliderId);

struct ActorInfo
{
    Util::Array<physx::PxShape*> shapes;
    CollisionFeedbackFlag feedbackFlag;
    uint16_t collisionGroup;
    float density;
    bool dynamic;
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
    ActorId CreateActorInstance(ActorResourceId id, Math::matrix44 const & trans, IndexT scene = 0);
      

private:
    

    /// perform actual load, override in subclass
    LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream);
    /// unload resource
    void Unload(const Resources::ResourceId id);


    Ids::IdAllocatorSafe<ActorInfo> allocator;
    __ImplementResourceAllocatorTypedSafe(allocator, ActorIdType);

};

extern StreamActorPool *actorPool;

} // namespace Physics