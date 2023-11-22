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
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "ids/id.h"
#include "core/debug.h"
namespace Resources
{
typedef Util::StringAtom ResourceName;
ID_24_8_24_8_NAMED_TYPE(ResourceId, loaderInstanceId, loaderIndex, resourceId, resourceType);               // 24 bits: loader resource id, 8 bits: loader index, 24 bits: allocator id, 8 bits: allocator type

// define a generic typed ResourceId, this is so we can have specialized allocators, but have a common pool implementation...
ID_24_8_NAMED_TYPE(ResourceUnknownId, resourceId, resourceType);

} // namespace Resource

#define RESOURCE_ID_TYPE(type) struct type : public Resources::ResourceUnknownId { \
        constexpr type() {};\
        constexpr type(const Resources::ResourceUnknownId& res) : Resources::ResourceUnknownId(res) {};\
        constexpr type(const Resources::ResourceId& res) : Resources::ResourceUnknownId(res.resourceId, res.resourceType) {};\
        constexpr type(const Ids::Id24 id, const Ids::Id8 type) : Resources::ResourceUnknownId(id, type) {};\
    }; \
    static constexpr type Invalid##type = Resources::InvalidResourceUnknownId;
