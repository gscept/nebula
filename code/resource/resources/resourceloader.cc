//------------------------------------------------------------------------------
// resourceloader.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "resourceloader.h"
#include "io/ioserver.h"
#include "resourceserver.h"
#include "util/bit.h"

using namespace IO;
namespace Resources
{

__ImplementAbstractClass(Resources::ResourceLoader, 'RSLO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourceLoader::ResourceLoader() :
    async(false)
{
    // maybe this is arrogant, just 1024 pending resources (actual resources that is) per loader?
    this->pendingLoads.Reserve(1024);
    this->pendingStreamLods.Reserve(1024);
    this->pendingStreamQueue.SetSignalOnEnqueueEnabled(false);
    this->creatorThread = Threading::Thread::GetMyThreadId();
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::~ResourceLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**

*/
void
ResourceLoader::Setup()
{
    // implement loader-specific setups, such as placeholder and error resource ids, as well as the acceptable resource class
    this->uniqueResourceId = 0;

    // set the async flag in the constructor of your subclass implementation of the resource pool
    if (this->async)
    {
        this->streamerThread = ResourceLoaderThread::Create();
        this->streamerThread->SetName(this->streamerThreadName.Value());
        this->streamerThread->Start();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Discard()
{
    this->streamerThread->Stop();
    this->streamerThread = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceLoader::LoadFallbackResources()
{
    // load all placeholders
    for (IndexT i = 0; i < this->placeholders.Size(); i++)
    {
        this->placeholders[i].placeholderId = this->CreateResource(this->placeholders[i].placeholderName, nullptr, 0, "system"_atm, nullptr, nullptr, true, false);
        n_assert(this->placeholders[i].placeholderId != Resources::InvalidResourceId);
    }

    // load placeholder, don't load it async
    if (this->placeholderResourceName.IsValid())
    {
        this->placeholderResourceId = this->CreateResource(this->placeholderResourceName, nullptr, 0, "system"_atm, nullptr, nullptr, true, false);
        n_assert(this->placeholderResourceId != Resources::InvalidResourceId);
    }

    // load error, don't load it async
    if (this->failResourceName.IsValid())
    {
        this->failResourceId = this->CreateResource(this->failResourceName, nullptr, 0, "system"_atm, nullptr, nullptr, true, false);
        n_assert(this->failResourceId != Resources::InvalidResourceId);
    }
}

//------------------------------------------------------------------------------
/**
*/
uint
ResourceLoader::StreamResource(const ResourceId entry, IndexT frameIndex, uint requestedBits)
{
    // Assume the loader doesn't support streaming, whereby all data is loaded on initialize
    return 0xFFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
Resource::State
ResourceLoader::ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream)
{
    // Assume the loader doesn't support streaming, whereby all data is loaded on initialize
    return Resource::Failed;
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::SubresourceLoadStatus
ResourceLoader::StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
{
    return SubresourceLoadStatus::Full;
}

//------------------------------------------------------------------------------
/**
*/
uint
ResourceLoader::LodMask(const Ids::Id32 entry, float lod) const
{
    return 0xFFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::RequestLOD(const Ids::Id32 entry, float lod) const
{
    // Do nothing
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Update(IndexT frameIndex)
{
    IndexT i;
    for (i = this->pendingLoads.Size() - 1; i >= 0; i--)
    {
        // get pending element
        _PendingResourceLoad& resourceLoad = this->loads[this->pendingLoads[i]];
        resourceLoad.frame = frameIndex;

        // If already loaded, just return
        this->asyncSection.Enter();
        Resource::State state = this->states[resourceLoad.entry];
        this->asyncSection.Leave();
        if ((resourceLoad.mode == _PendingResourceLoad::None) && (state == Resource::Loaded || state == Resource::Failed))
        {
            this->pendingLoads.EraseIndexSwap(i);
            continue;
        }

        // Load resource async
        this->LoadAsync(resourceLoad);
        resourceLoad.mode = _PendingResourceLoad::None;
    }

    // go through pending lod streams
    this->pendingStreamQueue.DequeueAll(this->pendingStreamLods);
    for (i = 0; i < this->pendingStreamLods.Size(); i++)
    {
        const _PendingStreamLod& streamLod = this->pendingStreamLods[i];

        this->asyncSection.Enter();
        _PendingResourceLoad& load = this->loads[streamLod.id.loaderInstanceId];
        load.lod = streamLod.lod;
        load.mode |= _PendingResourceLoad::Update;

        // Update state to continue streaming
        this->pendingLoads.Append(streamLod.id.loaderInstanceId);
        this->asyncSection.Leave();
    }

    // go through pending unloads
    for (i = this->pendingUnloads.Size() - 1; i >= 0; i--)
    {
        const _PendingResourceUnload& unload = this->pendingUnloads[i];
        if (this->states[unload.resourceId.loaderInstanceId] != Resource::Pending)
        {
            if (this->states[unload.resourceId.loaderInstanceId] == Resource::Loaded)
            {
                // unload if loaded
                this->Unload(unload.resourceId);
                this->states[unload.resourceId.loaderInstanceId] = Resource::Unloaded;
            }

            // give up the resource id
            this->resourceInstanceIndexPool.Dealloc(unload.resourceId.loaderInstanceId);
            
            // remove pending unload if not Pending or Loaded (so explicitly Unloaded or Failed)
            this->pendingUnloads.EraseIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::SetupIdFromEntry(const Ids::Id32 entry, ResourceId& cacheEntry)
{
    const Resource::State state = this->states[entry];
    switch (state)
    {
        case Resource::State::Loaded:
            cacheEntry.resourceId = this->resources[entry].resourceId;
            cacheEntry.resourceType = this->resources[entry].resourceType;
            break;
        case Resource::State::Failed:
            cacheEntry.resourceId = this->failResourceId.resourceId;
            cacheEntry.resourceType = this->failResourceId.resourceType;
            break;
        case Resource::State::Pending:
        case Resource::State::Unloaded:
            cacheEntry.resourceId = this->placeholderResourceId.resourceId;
            cacheEntry.resourceType = this->placeholderResourceId.resourceType;
            break;
    }
}

//------------------------------------------------------------------------------
/**
    Run all callbacks pending on a resource, must be within the critical section!
*/
void
ResourceLoader::RunCallbacks(Resource::State status, const Resources::ResourceId id)
{
    Util::Array<_Callbacks>& cbls = this->callbacks[id.loaderInstanceId];
    IndexT i;
    for (i = cbls.Size() - 1; i >= 0; i--)
    {
        _Callbacks& cbl = cbls[i];
        if (status == Resource::Loaded && cbl.success != nullptr)	
            cbl.success(id);
        else if (status == Resource::Failed && cbl.failed != nullptr)
            cbl.failed(id);
        cbls.EraseIndexSwap(i);
        cbl = {};
    }
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceId
ResourceLoader::GetPlaceholder(const Resources::ResourceName& name)
{
    n_assert(this->placeholders.Size() > 0);
    return this->placeholders[0].placeholderId;
}

//------------------------------------------------------------------------------
/**
*/
Resource::State
_LoadInternal(ResourceLoader* loader, const ResourceLoader::_PendingResourceLoad& res)
{
    loader->asyncSection.Enter();
    Resource::State state = loader->states[res.entry];
    ResourceId originalResource = loader->resources[res.entry];
    ResourceName name = loader->names[res.entry];
    loader->asyncSection.Leave();

    ResourceLoader::_LoadMetaData& metaData = loader->metaData[res.entry];

    n_assert(originalResource.loaderInstanceId == res.entry);

    ResourceId resource = originalResource;
    uint requestedBits = loader->requestedBits[res.entry];
    uint loadedBits = 0x0;

    if (AllBits(res.mode, ResourceLoader::_PendingResourceLoad::Create))
    {
        // construct stream
        Ptr<Stream> stream = IO::IoServer::Instance()->CreateStream(name.Value());
        stream->SetAccessMode(Stream::ReadAccess);
        if (stream->Open())
        {
            // If new resource, initialize it
            ResourceUnknownId internalResource = loader->InitializeResource(res.entry, res.tag, stream, res.immediate);
            resource.resourceId = internalResource.resourceId;
            resource.resourceType = internalResource.resourceType;
            requestedBits = loader->LodMask(res.entry, res.lod);

            if (res.immediate)
            {
                loadedBits = requestedBits;
                state = Resource::Loaded;
                goto skip_stream;
            }

            if (internalResource == InvalidResourceUnknownId)
            {
                // If the initialize failed, it means the file is invalid or can't be found
                state = Resource::Failed;
                resource.resourceId = loader->failResourceId.resourceId;
                resource.resourceType = loader->failResourceId.resourceType;
                n_printf("[RESOURCE LOADER] Failed to load resource %s\n", loader->names[res.entry].Value());
                goto skip_stream;
            }
        }
        else
        {
            resource.resourceId = loader->failResourceId.resourceId;
            resource.resourceType = loader->failResourceId.resourceType;
            n_printf("[RESOURCE LOADER] Failed to open resource %s\n", loader->names[res.entry].Value());
            state = Resource::Failed;
            goto skip_stream;
        }
    }

    if (AllBits(res.mode, ResourceLoader::_PendingResourceLoad::Update))
    {
        requestedBits |= loader->LodMask(res.entry, res.lod);
    }

    // If successful, begin streaming its data
    loadedBits = loader->StreamResource(resource, res.frame, requestedBits);
    if (AllBits(loadedBits, requestedBits))
    {
        state = Resource::Loaded;
    }
    else
    {
        state = Resource::Pending;
    }

skip_stream:
    loader->asyncSection.Enter();
    loader->requestedBits[res.entry] = requestedBits;
    loader->loadedBits[res.entry] = loadedBits;
    loader->states[res.entry] = state;
    loader->resources[res.entry] = resource;

    // We run the callbacks if the resource loaded or failed
    if (state == Resource::Loaded || state == Resource::Failed)
        loader->RunCallbacks(state, resource);

    // free metadata
    if (metaData.data != nullptr)
    {
        Memory::Free(Memory::ScratchHeap, metaData.data);
        metaData.data = nullptr;
        metaData.size = 0;
    }
    loader->asyncSection.Leave();

    return state;
}

//------------------------------------------------------------------------------
/**
*/
Resource::State
ResourceLoader::LoadImmediate(_PendingResourceLoad& res)
{
    // If already loaded, just return
    if (this->states[res.entry] == Resource::Loaded)
        return Resource::Loaded;

    return _LoadInternal(this, res);
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::LoadAsync(_PendingResourceLoad& res)
{
    // Create callable function
    auto loadFunc = std::bind(_LoadInternal, this, res);

    // If async, add function call to thread
    if (this->async)
    {
        res.inflight = true;
        this->streamerThread->jobs.Enqueue(loadFunc);
    }
    else
    {
        // Otherwise, run immediately
        loadFunc();
    }
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceId
Resources::ResourceLoader::CreateResource(const ResourceName& res, const void* loadInfo, SizeT loadInfoSize, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate, bool stream)
{
    // this assert should maybe be removed in favor of putting things on a queue if called from another thread
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);

    IndexT i = this->ids.FindIndex(res);

    // Setup return value as placeholder by default
    ResourceId ret = this->placeholderResourceId;

    if (i == InvalidIndex)
    {
        // allocate new index for the allocator
        Ids::Id32 instanceId = this->resourceInstanceIndexPool.Alloc(); // this is the ID of the container

        // create new resource id, if need be, grow the container list
        if (instanceId >= (uint)this->names.Size())
        {
            this->names.Resize(this->names.Size() + ResourceIndexGrow);
            this->usage.Resize(this->usage.Size() + ResourceIndexGrow);
            this->tags.Resize(this->tags.Size() + ResourceIndexGrow);
            this->states.Resize(this->states.Size() + ResourceIndexGrow);
            this->requestedBits.Resize(this->requestedBits.Size() + ResourceIndexGrow);
            this->loadedBits.Resize(this->loadedBits.Size() + ResourceIndexGrow);
            this->resources.Resize(this->resources.Size() + ResourceIndexGrow);
            this->callbacks.Resize(this->callbacks.Size() + ResourceIndexGrow);
            this->loads.Resize(this->loads.Size() + ResourceIndexGrow);
            this->metaData.Resize(this->metaData.Size() + ResourceIndexGrow);
            this->streams.Resize(this->streams.Size() + ResourceIndexGrow);
        }

        // add the resource name to the resource id
        this->names[instanceId] = res;
        this->usage[instanceId] = 1;
        this->tags[instanceId] = tag;
        this->states[instanceId] = Resource::Pending;
        this->loadedBits[instanceId] = 0x0;
        this->requestedBits[instanceId] = 0xFFFFFFFF;

        // allocate metadata if present
        _LoadMetaData metaData;
        if (loadInfo != nullptr)
        {
            metaData.data = Memory::Alloc(Memory::ScratchHeap, loadInfoSize);
            metaData.size = loadInfoSize;
            memcpy(metaData.data, loadInfo, loadInfoSize);
        }
        else
        {
            metaData.data = nullptr;
            metaData.size = 0;
        }
        this->metaData[instanceId] = metaData;

        ret.loaderInstanceId = instanceId;
        ret.loaderIndex = this->uniqueId;
        this->resources[instanceId] = ret;

        _PendingResourceLoad pending;
        pending.entry = instanceId;
        pending.tag = tag;
        pending.inflight = false;
        pending.immediate = immediate;
        pending.reload = false;
        pending.lod = stream ? 1.0f : 0.0f;
        pending.mode = _PendingResourceLoad::Create;
        this->loads[instanceId] = pending;

        // add mapping between resource name and resource being loaded
        this->ids.Add(res, instanceId);

        if (immediate)
        {
            Resource::State result = this->LoadImmediate(pending);
            SetupIdFromEntry(pending.entry, ret);
            if (result == Resource::Loaded && success != nullptr)
                success(ret);
            else if (result == Resource::Failed && failed != nullptr)
                failed(ret);
        }
        else
        {
            this->pendingLoads.Append(instanceId);
            if (success != nullptr || failed != nullptr)
            {
                // we need not worry about the thread, since this resource is new
                this->callbacks[instanceId].Append({ ret, success, failed });
            }
        }
    }
    else // this means the resource container is already created, and it may or may not be pending
    {
        // Get id of previously created resource
        Ids::Id32 instanceId = this->ids.ValueAtIndex(i);

        // bump usage
        this->usage[instanceId]++;

        // start the async section, the loader might change the resource state
        this->asyncSection.Enter();
        ret = this->resources[instanceId];
        
        // If the resource isn't pending, call the fail or success callback immediately
        const Resource::State state = this->states[instanceId];
        if (state == Resource::Loaded && success != nullptr)
            success(ret);
        else if (state == Resource::Failed && failed != nullptr)
            failed(ret);
        else if (state == Resource::Pending)
        {
            // this resource should now be in the pending list
            n_assert(i != InvalidIndex);

            // pending resource may not be in-flight in thread
            _PendingResourceLoad& pend = this->loads[instanceId];
            if (!pend.inflight)
            {
                // flip the immediate flag, this is in case we decide to perform a later load using immediate override
                pend.immediate = pend.immediate || immediate;
            }

            // since we are pending and inside the async section, it means the resource is not loaded yet, which means its safe to add the callback
            this->callbacks[instanceId].Append({ ret, success, failed });
        }
        else if (state == Resource::Unloaded)
        {
            // allocate metadata if present
            _LoadMetaData metaData;
            if (loadInfo != nullptr)
            {
                metaData.data = Memory::Alloc(Memory::ScratchHeap, loadInfoSize);
                metaData.size = loadInfoSize;
                memcpy(metaData.data, loadInfo, loadInfoSize);
            }
            else
            {
                metaData.data = nullptr;
                metaData.size = 0;
            }
            this->metaData[instanceId] = metaData;

            _PendingResourceLoad pending;
            pending.entry = instanceId;
            pending.tag = tag;
            pending.inflight = false;
            pending.immediate = immediate;
            pending.reload = false;
            pending.lod = stream ? 1.0f : 0.0f;
            pending.mode = _PendingResourceLoad::Create;
            this->states[instanceId] = Resource::Pending;
            this->loads[instanceId] = pending;

            if (immediate)
            {
                Resource::State result = this->LoadImmediate(pending);
                SetupIdFromEntry(pending.entry, ret);
                if (result == Resource::Loaded && success != nullptr)
                {
                    success(ret);
                }
                else if (result == Resource::Failed && failed != nullptr)
                {
                    ret = this->failResourceId;
                    failed(ret);
                }
            }
            else
            {
                this->pendingLoads.Append(instanceId);
                if (success != nullptr || failed != nullptr)
                {
                    // if unloaded, the callbacks array can safetly be assumed to be empty
                    this->callbacks[instanceId].Append({ ret, success, failed });
                }
            }
        }

        // leave async section
        this->asyncSection.Leave();
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
Resources::ResourceLoader::DiscardResource(const Resources::ResourceId id)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    if (id != this->placeholderResourceId && id != this->failResourceId)
    {
        // if usage reaches 0, add it to the list of pending unloads
        if (this->usage[id.loaderInstanceId] == 0)
        {
            if (this->async)
            {
                // add pending unload, it will be unloaded once loaded
                this->pendingUnloads.Append({ id });
            
            }
            else
            {
                this->Unload(id);
                this->states[id.loaderInstanceId] = Resource::Unloaded;
            }
            this->resourceInstanceIndexPool.Dealloc(id.loaderInstanceId);
        }
    }
#if N_DEBUG
    else
    {
        n_warning("Trying to delete placeholder or fail resource!\n");
    }
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::DiscardByTag(const Util::StringAtom& tag)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    IndexT i;
    for (i = 0; i < this->tags.Size(); i++)
    {
        if (this->tags[i] == tag)
        {
            // add pending unload, it will be unloaded once loaded
            this->pendingUnloads.Append({ this->resources[this->ids[this->names[i]]] });
            this->tags[i] = "";
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::CreateListener(
    const Resources::ResourceId res,
    std::function<void(const Resources::ResourceId)> success,
    std::function<void(const Resources::ResourceId)> failed
)
{
    // this assert should maybe be removed in favor of putting things on a queue if called from another thread
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);

    this->asyncSection.Enter();
    n_assert(this->states.Size() > res.loaderInstanceId);
    const auto state = this->states[res.loaderInstanceId];
    if (state == Resource::Loaded)
    {
        if (success != nullptr)
            success(res);
    }
    else if (state == Resource::Failed)
    {
        if (failed != nullptr)
            failed(this->failResourceId);
    }
    else
    {
        this->callbacks[res.loaderInstanceId].Append(_Callbacks {res, success, failed});
    }
    this->asyncSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceLoader::ReloadResource(const Resources::ResourceName& res, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    IndexT i = this->ids.FindIndex(res);
    if (i != InvalidIndex)
    {
        // get id of resource
        Resources::ResourceId ret = this->ids.ValueAtIndex(i);

        _PendingResourceLoad pending;
        pending.entry = ret.loaderInstanceId;
        pending.tag = this->tags[ret.loaderInstanceId];
        pending.inflight = false;
        pending.immediate = false;
        pending.reload = true;
        pending.lod = 1.0f;
        pending.mode = _PendingResourceLoad::Create;

        this->asyncSection.Enter();
        this->loads[ret.loaderInstanceId] = pending;
        this->states[ret.loaderInstanceId] = Resource::Pending;
        this->callbacks[ret.loaderInstanceId].Append({ ret, success, failed });
        this->asyncSection.Leave();
    }
    else
    {
        n_warning("Resource '%s' has to be loaded before it can be reloaded\n", res.AsString().AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceLoader::ReloadResource(const Resources::ResourceId& id, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    n_assert_fmt(id != Resources::ResourceId::Invalid(), "Resource %d is not loaded, it has to be before it can be reloaded", id.HashCode());

    _PendingResourceLoad pending;
    pending.entry = id.loaderInstanceId;
    pending.tag = this->tags[id.loaderInstanceId];
    pending.inflight = false;
    pending.immediate = false;
    pending.reload = true;
    pending.lod = 1.0f;
    pending.mode = _PendingResourceLoad::Create;

    this->asyncSection.Enter();
    this->loads[id.loaderInstanceId] = pending;
    this->states[id.loaderInstanceId] = Resource::Pending;
    this->callbacks[id.loaderInstanceId].Append({ id, success, failed });
    this->asyncSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceLoader::SetMinLod(const Resources::ResourceId& id, const float lod, bool immediate)
{
    if (immediate)
    {
        this->StreamMaxLOD(id, lod, immediate);
    }
    else
    {
        _PendingStreamLod pending;
        pending.id = id;
        pending.lod = lod;
        pending.immediate = immediate;
        this->pendingStreamQueue.Enqueue(pending);
    }
}

} // namespace Resources
