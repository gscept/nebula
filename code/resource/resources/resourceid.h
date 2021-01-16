#pragma once
//------------------------------------------------------------------------------
/**
    The ResourceId type is just a StringAtom, but is primarily meant for resources.

    The first 32 bits of the resource id is used internally for the pool to relate the shared
    resource with the pool, since a pool can share allocator with another pool. The 24 following 
    bytes is the actual resource handle, which can be used with the many different pools to fetch the
    data underlying that resource. The last 8 bits is the type of the loader, which makes it 
    fast to lookup and delete said resource instance.

    24 bytes ---------------8 bytes --------------- 24 bytes -------------- 8 bytes
    Pool storage id         Pool id                 Allocator id            Allocator resource type

    The pool storage id is the resource level index, the pool id is the global id of the resource pool, 
    allocator id is instance level id, and the allocator resource type is just a flag we can use to check
    the type of the id, in case we are confused. 

    Example: If we allocate a texture and want to use TextureId, then we take the 24 bytes part (id.id24)
    and use it to construct a Texture id, but we still need the ResourceId if we want to deallocate that
    texture at some later point. To convert to such an id, use the SpecializedId.
    
    (C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "ids/id.h"
#include "core/debug.h"
namespace Resources
{
typedef Util::StringAtom ResourceName;
ID_24_8_24_8_NAMED_TYPE(ResourceId, poolId, poolIndex, resourceId, resourceType);               // 24 bits: pool-resource id, 8 bits: pool id, 24 bits: allocator id, 8 bits: allocator type

// define a generic typed ResourceId, this is so we can have specialized allocators, but have a common pool implementation...
ID_24_8_TYPE(ResourceUnknownId);

} // namespace Resource

#define RESOURCE_ID_TYPE(type) struct type : public Resources::ResourceId { \
        constexpr type() {};\
        constexpr type(const Resources::ResourceId& res) : Resources::ResourceId(res) {};\
    }; \
    static constexpr type Invalid##type = Resources::InvalidResourceId;

// use this struct to pass a loading package through to a subsystem, like skeleton, animation, etc
struct ResourceCreateInfo
{
    Resources::ResourceName resource;
    Util::StringAtom tag;
    std::function<void(const Resources::ResourceId)> successCallback;
    std::function<void(const Resources::ResourceId)> failCallback;
    bool async;
};
