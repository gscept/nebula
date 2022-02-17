#pragma once
//------------------------------------------------------------------------------
/**
    The resource pool implements a resource specific pool, which implements
    loading, unloading and fetching of resource.

    To implement a custom pool, look at ResourceStreamCache for file loading,
    and ResourceMemoryCache for memory loading.

    For ResourceStreamCache, Load and Unload must be implemented.
    For ResourceMemoryCache, LoadFromMemory and Unload must be implemented.

    For both, the following functions must be implemented:
        AllocObject
        DeallocObject

    The ResourceAllocator class lets you implement an efficient resource allocator
    straight away, by simply putting the __ResourceAllocator macro in your class,
    however it is also viable to implement your own ResourceAllocator type if 
    necessary. This macro implements the Alloc and Dealloc functions.

    Use the __ResourceAllocatorType to construct an allocator, it takes a list
    of types which is then implemented as separate arrays. Couple it with the
    __ImplementResourceAllocator to implement the allocator functions.

    is not, because the comma after uint32_t will end the first macro argument, and Util::Array<VkDescriptorSetLayoutBinding>>
    becomes the second, and setBindings the third (when the macro only takes two arguments).

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "ids/idpool.h"
#include "ids/idallocator.h"
#include "resource.h"
#include "resourceid.h"
#include <tuple>


// these macros implement different styles for the resource allocators, the resource-versions of get is just for convenience

#define __ImplementResourceAllocator(name) \
    inline Resources::ResourceUnknownId AllocObject() { return Ids::Id::MakeId24_8(name.Alloc(), 0xFF); } \
    inline void DeallocObject(const Resources::ResourceUnknownId id) { name.Dealloc(id.id24); } \
    template<int MEMBER> inline auto& Get(const Ids::Id24 id) { return name.Get<MEMBER>(id); } \
    template<int MEMBER> inline auto& Get(const Resources::ResourceId id) { return name.Get<MEMBER>(id.resourceId); }

#define __ImplementResourceAllocatorSafe(name) \
    inline Resources::ResourceUnknownId AllocObject() { return Ids::Id::MakeId24_8(name.AllocObject(), 0xFF); } \
    inline void DeallocObject(const Resources::ResourceUnknownId id) { name.DeallocObject(id.id24); } \
    inline void Lock(Util::ArrayAllocatorAccess access) { name.Lock(access); } \
    inline void Unlock(Util::ArrayAllocatorAccess access) { name.Unlock(access); } \
    template<int MEMBER> inline auto& Get(const Ids::Id24 id) { return name.Get<MEMBER>(id); } \
    template<int MEMBER> inline auto& Get(const Resources::ResourceId id) { return name.Get<MEMBER>(id.resourceId); } \
    template<int MEMBER> inline auto& GetUnsafe(const Ids::Id24 id) { return name.GetUnsafe<MEMBER>(id); } \
    template<int MEMBER> inline auto& GetUnsafe(const Resources::ResourceId id) { return name.GetUnsafe<MEMBER>(id.resourceId); } \
    template<int MEMBER> inline auto& GetSafe(const Ids::Id24 id) { return name.GetSafe<MEMBER>(id); } \
    template<int MEMBER> inline auto& GetSafe(const Resources::ResourceId id) { return name.GetSafe<MEMBER>(id.resourceId); }

#define __ImplementResourceAllocatorTyped(name, idtype) \
    inline Resources::ResourceUnknownId AllocObject() { return Ids::Id::MakeId24_8(name.Alloc(), idtype); } \
    inline void DeallocObject(const Resources::ResourceUnknownId id) { name.Dealloc(id.id24); } \
    template<int MEMBER> inline auto& Get(const Ids::Id24 id) { return name.Get<MEMBER>(id); } \
    template<int MEMBER> inline auto& Get(const Resources::ResourceId id) { return name.Get<MEMBER>(id.resourceId); }

#define __ImplementResourceAllocatorTypedSafe(name, idtype) \
    inline Resources::ResourceUnknownId AllocObject() { return Ids::Id::MakeId24_8(name.Alloc(), idtype); } \
    inline void DeallocObject(const Resources::ResourceUnknownId id) { name.Dealloc(id.id24); } \
    inline void Lock(Util::ArrayAllocatorAccess access) { name.Lock(access); } \
    inline void Unlock(Util::ArrayAllocatorAccess access) { name.Unlock(access); } \
    inline auto* Allocator() { return &name; } \
    template<int MEMBER> inline auto& Get(const Ids::Id24 id) { return name.Get<MEMBER>(id); } \
    template<int MEMBER> inline auto& Get(const Resources::ResourceId id) { return name.Get<MEMBER>(id.resourceId); } \
    template<int MEMBER> inline auto& GetUnsafe(const Ids::Id24 id) { return name.GetUnsafe<MEMBER>(id); } \
    template<int MEMBER> inline auto& GetUnsafe(const Resources::ResourceId id) { return name.GetUnsafe<MEMBER>(id.resourceId); } 

namespace Resources
{
class ResourceCache : public Core::RefCounted
{
    __DeclareAbstractClass(ResourceCache);
public:
    /// constructor
    ResourceCache();
    /// destructor
    virtual ~ResourceCache();

    /// setup resource loader
    virtual void Setup();
    /// discard resource loader
    virtual void Discard();

    /// loads error and placeholder resources if valid
    virtual void LoadFallbackResources();

    /// discard resource instance
    virtual void DiscardResource(const Resources::ResourceId id);
    /// discard all resources associated with a tag
    virtual void DiscardByTag(const Util::StringAtom& tag);

    /// get resource name
    const Resources::ResourceName& GetName(const Resources::ResourceId id) const;
    /// get resource usage from resource id
    const uint32_t GetUsage(const Resources::ResourceId id) const;
    /// get resource tag was first registered with
    const Util::StringAtom GetTag(const Resources::ResourceId id) const;
    /// get resource state
    const Resource::State GetState(const Resources::ResourceId id) const;
    /// get resource id by name, use with care
    const Resources::ResourceId GetId(const Resources::ResourceName& name) const;
    /// get the dictionary of all resource-id pairs
    const Util::Dictionary<Resources::ResourceName, Resources::ResourceId>& GetResources() const;
    /// returns true if pool has resource
    const bool HasResource(const Resources::ResourceId id) const;

    /// get the global identifier for this pool
    const int32_t& GetUniqueId() const;

    /// update the resource loader, this is done every frame
    virtual void Update(IndexT frameIndex);

    enum LoadStatus
    {
        Success,        /// resource is properly loaded
        Failed,         /// resource loading failed
        Delay,          /// resource is loaded at some later point
        Threaded        /// resource is loaded from a thread, which is like Delay, but is no longer pending
    };
    static const uint32_t ResourceIndexGrow = 512;

protected:
    friend class ResourceServer;

    /// request new resource and generate id for it, implement in subclass
    virtual ResourceUnknownId AllocObject() = 0;
    /// deallocate resource
    virtual void DeallocObject(const ResourceUnknownId id) = 0;
    /// unload resource (overload to implement resource deallocation)
    virtual void Unload(const Resources::ResourceId id) = 0;

    /// id in resource manager
    int32_t uniqueId;

    Util::Dictionary<Resources::ResourceName, Resources::ResourceId> ids;
    Ids::IdPool resourceInstanceIndexPool;

    Util::FixedArray<Resources::ResourceName> names;
    Util::FixedArray<uint32_t> usage;
    Util::FixedArray<Util::StringAtom> tags;
    Util::FixedArray<Resource::State> states;
    uint32_t uniqueResourceId;
};

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
ResourceCache::GetName(const Resources::ResourceId id) const
{
    return this->names[id.poolId];
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
ResourceCache::GetUsage(const Resources::ResourceId id) const
{
    return this->usage[id.poolId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom
ResourceCache::GetTag(const Resources::ResourceId id) const
{
    return this->tags[id.poolId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::Resource::State
ResourceCache::GetState(const Resources::ResourceId id) const
{
    return this->states[id.poolId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId
ResourceCache::GetId(const Resources::ResourceName& name) const
{
    IndexT i = this->ids.FindIndex(name);
    if (i == InvalidIndex)  return Resources::ResourceId::Invalid();
    else                    return this->ids.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Resources::ResourceName, Resources::ResourceId>&
ResourceCache::GetResources() const
{
    return this->ids;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
ResourceCache::HasResource(const Resources::ResourceId id) const
{
    return this->names.Size() > (SizeT)id.poolId;
}

//------------------------------------------------------------------------------
/**
*/
inline const int32_t&
ResourceCache::GetUniqueId() const
{
    return this->uniqueId;
}
} // namespace Resources
