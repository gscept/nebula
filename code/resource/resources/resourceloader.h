#pragma once
//------------------------------------------------------------------------------
/**
    Loads resources as streams and is updated for every ResourceServer::Update().

    Contains the names for the placeholder and failed-to-load resource names.
    When inheriting from this class, make sure to provide proper resource ids for:
        1. Placeholder resource
        2. Error resource

    If no placeholder resource is provided, the loader cannot execute asynchronously.
    If no error resource is provided and the resource fails to load, then the ResourceServer
    will raise an assertion. 

    Each resource pool also keeps a list of the resources loaded by it. Therefore,
    the ResourceServer is not responsible for maintaining which resources are loaded.

    The pool associates a resource name (StringAtom) with an id, such that it can be quickly
    retrieved. 
    
    When creating an instance of a resource, an ID is returned, this ID contains the following:
    32 bits (resource instance id), 24 bits (resource id) and 8 bits (loader id). The instance id
    is a recyclable number which uniquely identifies a single allocation. The next 24 bits is the
    internal ID for the resource, which is only loaded once. The last 8 bits identifies which loader
    created the resource. 

    Resources created with tags must also be removed using the tag. A tagged resource can only
    be discarded by using that tag. If a resource is loaded with a tag, it will remain bound
    to that tag, no matter what consecutive loads say. 
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/stringatom.h"
#include "io/stream.h"
#include "util/set.h"
#include "resource.h"
#include "threading/safequeue.h"
#include "threading/threadid.h"
#include "ids/idpool.h"
#include <tuple>
#include <functional>

namespace Resources
{
class Resource;
class ResourceLoaderThread;
class ResourceLoader : public Core::RefCounted
{
    __DeclareAbstractClass(ResourceLoader);

public:
    /// constructor
    ResourceLoader();
    /// destructor
    virtual ~ResourceLoader();

    /// setup resource loader, initiates the placeholder and error resources if valid, so don't forget to run!
    virtual void Setup();
    /// discard resource loader
    virtual void Discard();

    /// load placeholder and error resources
    virtual void LoadFallbackResources();

    /// create a container with a tag associated with it, if no tag is provided, the resource will be untagged
    Resources::ResourceId CreateResource(const Resources::ResourceName& res, const void* loadInfo, SizeT loadInfoSize, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate);
    /// discard container
    void DiscardResource(const Resources::ResourceId id);
    /// discard all resources associated with a tag
    void DiscardByTag(const Util::StringAtom& tag);

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
    const Util::Dictionary<Resources::ResourceName, Ids::Id32>& GetResources() const;
    /// returns true if pool has resource
    const bool HasResource(const Resources::ResourceId id) const;

    /// get the global identifier for this pool
    const int32_t& GetUniqueId() const;

    /// reload resource using resource name
    void ReloadResource(const Resources::ResourceName& res, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed);
    /// reload resource using resource id
    void ReloadResource(const Resources::ResourceId& id, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed);

    /// begin updating a resources lod
    void SetMinLod(const Resources::ResourceId& id, const float lod, bool immediate);

protected:
    friend class ResourceServer;

    /// struct for pending resources which are about to be loaded
    struct _PendingResourceLoad
    {
        Ids::Id32 entry;
        Util::StringAtom tag;
        bool inflight;
        bool immediate;

        _PendingResourceLoad() : entry(-1) {};
    };

    /// struct for pending stream
    struct _PendingStreamLod
    {
        Resources::ResourceId id;
        float lod;
        bool immediate;

        _PendingStreamLod() : id(ResourceId::Invalid()) {};
    };

    struct _PendingResourceUnload
    {
        Resources::ResourceId resourceId;
    };

    /// callback functions to run when an associated resource is loaded (can be stacked)
    struct _Callbacks
    {
        Resources::ResourceId id;
        std::function<void(const Resources::ResourceId)> success;
        std::function<void(const Resources::ResourceId)> failed;
    };

    struct _LoadMetaData
    {
        void* data;
        SizeT size;
    };

    enum LoadStatus
    {
        Success,        /// resource is properly loaded
        Failed,         /// resource loading failed
        Delay,          /// resource is loaded at some later point
        Threaded        /// resource is loaded from a thread, which is like Delay, but is no longer pending
    };

    struct _InternalLoadResult
    {
        LoadStatus status;
        ResourceId id;
    };

    static const uint32_t ResourceIndexGrow = 512;

    /// perform actual load, override in subclass
    virtual ResourceUnknownId LoadFromStream(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) = 0;
    /// perform a reload
    virtual LoadStatus ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream);
    /// perform a lod update
    virtual void StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate);

    /// unload resource (overload to implement resource deallocation)
    virtual void Unload(const Resources::ResourceId id) = 0;
    /// update the resource loader, this is done every frame
    virtual void Update(IndexT frameIndex);

    /// Load immediately
    _InternalLoadResult LoadImmediate(_PendingResourceLoad& res);
    /// Load async
    LoadStatus LoadDeferred(_PendingResourceLoad& res);
    /// run callbacks
    void RunCallbacks(LoadStatus status, const Resources::ResourceId id);

    struct _PlaceholderResource
    {
        Resources::ResourceName placeholderName;
        Resources::ResourceId placeholderId;
    };
    Util::FixedArray<_PlaceholderResource> placeholders;

    /// get placeholder based on resource name
    Resources::ResourceId GetPlaceholder(const Resources::ResourceName& name);

    /// these types need to be properly initiated in a subclass Setup function
    Util::StringAtom placeholderResourceName;
    Util::StringAtom failResourceName;

    Resources::ResourceId placeholderResourceId;
    Resources::ResourceId failResourceId;

    struct StreamData
    {
        void* data;
        Ptr<IO::Stream> stream;
    };

    bool async;

    Ptr<ResourceLoaderThread> streamerThread;
    Util::StringAtom streamerThreadName;

    Util::Array<IndexT> pendingLoads;
    Util::Array<_PendingResourceUnload> pendingUnloads;
    Util::Array<_PendingStreamLod> pendingStreamLods;
    Threading::SafeQueue<_PendingStreamLod> pendingStreamQueue;

    Util::Dictionary<Resources::ResourceName, uint32_t> ids;
    Ids::IdPool resourceInstanceIndexPool;

    Util::FixedArray<Resources::ResourceName> names;
    Util::FixedArray<uint32_t> usage;
    Util::FixedArray<Util::StringAtom> tags;
    Util::FixedArray<Resource::State> states;
    Util::FixedArray<ResourceId> resources;
    Util::FixedArray<Util::Array<_Callbacks>> callbacks;
    Util::FixedArray<_PendingResourceLoad> loads;
    Util::FixedArray<_LoadMetaData> metaData;
    Util::FixedArray<StreamData> streams;
    uint32_t uniqueResourceId;

    /// id in resource manager
    int32_t uniqueId;

    /// async section to sync callbacks and pending list with thread
    Threading::CriticalSection asyncSection;
    Threading::ThreadId creatorThread;
};

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
ResourceLoader::GetName(const Resources::ResourceId id) const
{
    return this->names[id.cacheInstanceId];
}

//------------------------------------------------------------------------------
/**
*/
inline const uint32_t
ResourceLoader::GetUsage(const Resources::ResourceId id) const
{
    return this->usage[id.cacheInstanceId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom
ResourceLoader::GetTag(const Resources::ResourceId id) const
{
    return this->tags[id.cacheInstanceId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::Resource::State
ResourceLoader::GetState(const Resources::ResourceId id) const
{
    return this->states[id.cacheInstanceId];
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId
ResourceLoader::GetId(const Resources::ResourceName& name) const
{
    IndexT i = this->ids.FindIndex(name);
    if (i == InvalidIndex)  return Resources::ResourceId::Invalid();
    else                    return this->resources[this->ids.ValueAtIndex(i)];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Resources::ResourceName, Ids::Id32>&
ResourceLoader::GetResources() const
{
    return this->ids;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
ResourceLoader::HasResource(const Resources::ResourceId id) const
{
    return this->names.Size() > (SizeT)id.cacheInstanceId;
}

//------------------------------------------------------------------------------
/**
*/
inline const int32_t&
ResourceLoader::GetUniqueId() const
{
    return this->uniqueId;
}

} // namespace Resources
