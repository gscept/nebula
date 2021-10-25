#pragma once
//------------------------------------------------------------------------------
/**
    Implements a resource loader for physics shapes
    Can be either primitive shapes or meshes saved in seperate files

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resourcestreamcache.h"
#include "util/stack.h"
#include "physics/actorcontext.h"
#include "physics/streamactorpool.h"
#include "flat/physics/collider.h"
#include "physicsinterface.h"

namespace Physics
{    
struct ColliderInfo
{
    ColliderType type;
    Util::StringAtom material;
    physx::PxGeometryHolder geometry;        
};


class StreamColliderPool : public Resources::ResourceStreamCache
{
    __DeclareClass(StreamColliderPool);
public:
    /// constructor
    StreamColliderPool();
    /// destructor
    virtual ~StreamColliderPool();

    /// setup resource loader, initiates the placeholder and error resources if valid
    void Setup();

    ///
    physx::PxGeometryHolder & GetGeometry(ColliderId id);

private:


    /// perform actual load, override in subclass
    LoadStatus LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate);
    /// unload resource
    void Unload(const Resources::ResourceId id);


    Ids::IdAllocatorSafe<ColliderInfo> allocator;
    __ImplementResourceAllocatorTypedSafe(allocator, ColliderIdType);

};

extern StreamColliderPool * colliderPool;
} // namespace Physics