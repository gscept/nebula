//------------------------------------------------------------------------------
// resourceloader.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "resourceloader.h"
#include "io/ioserver.h"
#include "resourceserver.h"

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
        this->placeholders[i].placeholderId = this->CreateResource(this->placeholders[i].placeholderName, nullptr, 0, "system"_atm, nullptr, nullptr, true);
        n_assert(this->placeholders[i].placeholderId != Resources::InvalidResourceId);
    }

    // load placeholder, don't load it async
    if (this->placeholderResourceName.IsValid())
    {
        this->placeholderResourceId = this->CreateResource(this->placeholderResourceName, nullptr, 0, "system"_atm, nullptr, nullptr, true);
        n_assert(this->placeholderResourceId != Resources::InvalidResourceId);
    }

    // load error, don't load it async
    if (this->failResourceName.IsValid())
    {
        this->failResourceId = this->CreateResource(this->failResourceName, nullptr, 0, "system"_atm, nullptr, nullptr, true);
        n_assert(this->failResourceId != Resources::InvalidResourceId);
    }
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::LoadStatus 
ResourceLoader::ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream)
{
    return ResourceLoader::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceLoader::StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
{
    // implement in subclass
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

        // load resource, get status from load function
        LoadStatus result = this->LoadDeferred(resourceLoad);
        if (result != Delay) // if the loader tells us to delay, we delay the loading to next frame
            this->pendingLoads.EraseIndex(i);
        if (result != Threaded) // if it is an immediate load, run the callbacks directly
            this->RunCallbacks(result, this->resources[resourceLoad.entry]);
    }

    // go through pending lod streams
    this->pendingStreamQueue.DequeueAll(this->pendingStreamLods);
    for (i = this->pendingStreamLods.Size() - 1; i >= 0; i--)
    {
        const _PendingStreamLod& streamLod = this->pendingStreamLods[i];

        // skip this resource if not loaded, but keep it in the list
        this->asyncSection.Enter();
        bool resourceNotLoaded = this->states[streamLod.id.cacheInstanceId] != Resource::Loaded;
        this->asyncSection.Leave();
        if (resourceNotLoaded)
            continue;

        // if async is supported, put the stream job on a thread!
        if (this->async && !streamLod.immediate)
        {
            auto streamFunc = [this, streamLod]()
            {
                this->StreamMaxLOD(streamLod.id, streamLod.lod, streamLod.immediate);
            };
            this->streamerThread->jobs.Enqueue(streamFunc);

            this->pendingStreamLods.EraseIndex(i);
        }
        else
        {
            // load immediately
            this->StreamMaxLOD(streamLod.id, streamLod.lod, streamLod.immediate);

            this->pendingStreamLods.EraseIndex(i);
        }
    }

    // go through pending unloads
    for (i = this->pendingUnloads.Size() - 1; i >= 0; i--)
    {
        const _PendingResourceUnload& unload = this->pendingUnloads[i];
        if (this->states[unload.resourceId.cacheInstanceId] != Resource::Pending)
        {
            if (this->states[unload.resourceId.cacheInstanceId] == Resource::Loaded)
            {
                // unload if loaded
                this->Unload(unload.resourceId);
                this->states[unload.resourceId.cacheInstanceId] = Resource::Unloaded;
            }

            // give up the resource id
            this->resourceInstanceIndexPool.Dealloc(unload.resourceId.cacheInstanceId);
            
            // remove pending unload if not Pending or Loaded (so explicitly Unloaded or Failed)
            this->pendingUnloads.EraseIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Run all callbacks pending on a resource, must be within the critical section!
*/
void
ResourceLoader::RunCallbacks(LoadStatus status, const Resources::ResourceId id)
{
    Util::Array<_Callbacks>& cbls = this->callbacks[id.cacheInstanceId];
    IndexT i;
    for (i = 0; i < cbls.Size(); i++)
    {
        const _Callbacks& cbl = cbls[i];
        if (status == Success && cbl.success != nullptr)	
            cbl.success(id);
        else if (status == Failed && cbl.failed != nullptr)
        {
            Resources::ResourceId fail = id;
            fail.resourceId = this->failResourceId.resourceId;
            cbl.failed(fail);
        }
    }
    cbls.Clear();
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
ResourceLoader::_InternalLoadResult
ResourceLoader::LoadImmediate(_PendingResourceLoad& res)
{
    _InternalLoadResult ret;
    ret.id.cacheInstanceId = res.entry;
    ret.id.cacheIndex = this->uniqueId;
    ret.id.resourceId = this->failResourceId.resourceId;
    ret.id.resourceType = this->failResourceId.resourceType;

    // If already loaded, just return
    if (this->states[res.entry] == Resource::Loaded)
    {
        ret.status = Success;
        ret.id.resourceId = this->resources[res.entry].resourceId;
        ret.id.resourceType = this->resources[res.entry].resourceType;
        return ret;
    }

    // construct stream
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(this->names[res.entry].Value());
    stream->SetAccessMode(Stream::ReadAccess);

    _LoadMetaData& metaData = this->metaData[res.entry];
    if (stream->Open())
    {
        ResourceUnknownId result = this->LoadFromStream(res.entry, res.tag, stream, res.immediate);
        LoadStatus status = result == InvalidResourceUnknownId ? LoadStatus::Failed : LoadStatus::Success;
        ret.status = status;
        ret.id.resourceId = result.resourceId;
        ret.id.resourceType = result.resourceType;

        this->asyncSection.Enter();

        // If loaded, update the resource id
        if (status == Success)
        {
            this->states[res.entry] = Resource::Loaded;
        }
        else if (status == Failed)
            this->states[res.entry] = Resource::Failed;

        // Run callbacks and update entry
        this->RunCallbacks(status, ret.id);
        this->resources[res.entry] = ret.id;

        this->asyncSection.Leave();
    }
    else
    {
        this->asyncSection.Enter();
        this->RunCallbacks(Failed, ret.id);
        this->states[res.entry] = Resource::Failed;
        this->resources[res.entry] = ret.id;
        this->asyncSection.Leave();
        ret.status = Failed;
        n_printf("Failed to load resource %s\n", this->names[res.entry].Value());
    }

    // free metadata
    if (metaData.data != nullptr)
    {
        Memory::Free(Memory::ScratchHeap, metaData.data);
        metaData.data = nullptr;
        metaData.size = 0;
    }

    this->asyncSection.Enter();

    // Update the entry for this resource

    this->asyncSection.Leave();

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::LoadStatus
ResourceLoader::LoadDeferred(_PendingResourceLoad& res)
{
    // If already loaded, just return
    if (this->states[res.entry] == Resource::Loaded)
        return Success;

    // wrap the loading process as a lambda function and pass it to the thread
    auto loadFunc = [this, &res]()
    {
        // construct stream
        Ptr<Stream> stream = IO::IoServer::Instance()->CreateStream(this->names[res.entry].Value());
        stream->SetAccessMode(Stream::ReadAccess);
        _LoadMetaData& metaData = this->metaData[res.entry];

        ResourceId resource;
        resource.cacheIndex = this->uniqueId;
        resource.cacheInstanceId = res.entry;
        resource.resourceId = this->failResourceId.resourceId;
        resource.resourceType = this->failResourceId.resourceType;

        // enter critical section
        if (stream->Open())
        {
            ResourceUnknownId result = this->LoadFromStream(res.entry, res.tag, stream, res.immediate);
            LoadStatus status = result == InvalidResourceUnknownId ? LoadStatus::Failed : LoadStatus::Success;
            resource.resourceId = result.resourceId;
            resource.resourceType = result.resourceType;

            this->asyncSection.Enter();
            if (status == Success)
                this->states[res.entry] = Resource::Loaded;
            else if (status == Failed)
            {
                n_printf("Failed to load resource %s\n", this->names[res.entry].Value());
                this->states[res.entry] = Resource::Failed;
            }
            this->RunCallbacks(status, resource);
            this->resources[res.entry] = resource;

            this->asyncSection.Leave();
        }
        else
        {
            // Not being able to open the stream constitutes a failure too
            this->asyncSection.Enter();
            this->RunCallbacks(Failed, resource);
            this->states[res.entry] = Resource::Failed;
            n_printf("Failed to load resource %s\n", this->names[res.entry].Value());
            this->asyncSection.Leave();
        }

        // free metadata
        if (metaData.data != nullptr)
        {
            Memory::Free(Memory::ScratchHeap, metaData.data);
            metaData.data = nullptr;
            metaData.size = 0;
        }
    };

    if (this->async)
    {
        res.inflight = true;
        ResourceServer::Instance()->loaderThread->jobs.Enqueue(loadFunc);
        return Threaded;
    }
    else
    {
        loadFunc();
        return this->states[res.entry] == Resource::Loaded ? Success : Failed;
    }
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceId
Resources::ResourceLoader::CreateResource(const ResourceName& res, const void* loadInfo, SizeT loadInfoSize, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
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

        ret.cacheInstanceId = instanceId;
        ret.cacheIndex = this->uniqueId;
        this->resources[instanceId] = ret;

        _PendingResourceLoad pending;
        pending.entry = instanceId;
        pending.tag = tag;
        pending.inflight = false;
        pending.immediate = immediate;
        this->loads[instanceId] = pending;

        // add mapping between resource name and resource being loaded
        this->ids.Add(res, instanceId);

        if (immediate)
        {
            _InternalLoadResult result = this->LoadImmediate(pending);
            if (result.status == Failed && failed != nullptr)
                failed(result.id);
            else if (result.status == Success && success != nullptr)
                success(result.id);

            ret = result.id;
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
        ret = this->resources[instanceId];

        // bump usage
        this->usage[ret.cacheInstanceId]++;

        // start the async section, the loader might change the resource state
        this->asyncSection.Enter();
        
        // If the resource isn't pending, call the fail or success callback immediately
        const Resource::State state = this->states[ret.cacheInstanceId];
        if (state == Resource::Loaded && success != nullptr)
            success(ret);
        else if (state == Resource::Failed && failed != nullptr)
            failed(ret);
        else if (state == Resource::Pending)
        {
            // this resource should now be in the pending list
            n_assert(i != InvalidIndex);

            // pending resource may not be in-flight in thread
            _PendingResourceLoad& pend = this->loads[ret.cacheInstanceId];
            if (!pend.inflight)
            {
                // flip the immediate flag, this is in case we decide to perform a later load using immediate override
                pend.immediate = pend.immediate || immediate;
            }

            // since we are pending and inside the async section, it means the resource is not loaded yet, which means its safe to add the callback
            this->callbacks[ret.cacheInstanceId].Append({ ret, success, failed });
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
            this->metaData[ret.cacheInstanceId] = metaData;

            _PendingResourceLoad pending;
            pending.entry = instanceId;
            pending.tag = tag;
            pending.inflight = false;
            pending.immediate = immediate;
            this->states[ret.cacheInstanceId] = Resource::Pending;
            this->loads[ret.cacheInstanceId] = pending;

            if (immediate)
            {
                _InternalLoadResult result = this->LoadImmediate(pending);
                ret = result.id;
            }
            else
            {
                this->pendingLoads.Append(ret.cacheInstanceId);
                if (success != nullptr || failed != nullptr)
                {
                    // if unloaded, the callbacks array can safetly be assumed to be empty
                    this->callbacks[ret.cacheInstanceId].Append({ ret, success, failed });
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
        ResourceLoader::DiscardResource(id);

        // if usage reaches 0, add it to the list of pending unloads
        if (this->usage[id.cacheInstanceId] == 0)
        {
            if (this->async)
            {
                // add pending unload, it will be unloaded once loaded
                this->pendingUnloads.Append({ id });
            
            }
            else
            {
                this->Unload(id);
                this->states[id.cacheInstanceId] = Resource::Unloaded;
            }
            this->resourceInstanceIndexPool.Dealloc(id.cacheInstanceId);
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
ResourceLoader::ReloadResource(const Resources::ResourceName& res, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    IndexT i = this->ids.FindIndex(res);
    if (i != InvalidIndex)
    {
        // get id of resource
        Resources::ResourceId ret = this->ids.ValueAtIndex(i);

        // copy the reference
        IoServer* ioserver = IoServer::Instance();

        // construct stream
        Ptr<Stream> stream = ioserver->CreateStream(this->names[ret.cacheInstanceId].Value());
        stream->SetAccessMode(Stream::ReadAccess);

        // enter critical section
        if (stream->Open())
        {
            LoadStatus stat = this->ReloadFromStream(ret, stream);
            this->asyncSection.Enter();
            if (stat == Success && success != nullptr)
                success(ret);
            else if (stat == Failed && failed != nullptr)
                failed(ret);
            this->asyncSection.Leave();

            // close stream
            stream->Close();
        }
        else
        {
            // if we fail to reload, just keep the old resource!
            this->asyncSection.Enter();
            if (failed != nullptr)
                failed(ret);
            this->asyncSection.Leave();
            n_printf("Failed to reload resource %s\n", this->names[ret.cacheInstanceId].Value());
        }
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

    // copy the reference
    IoServer* ioserver = IoServer::Instance();

    // construct stream
    Ptr<Stream> stream = ioserver->CreateStream(this->names[id.cacheInstanceId].Value());
    stream->SetAccessMode(Stream::ReadAccess);

    // enter critical section
    if (stream->Open())
    {
        LoadStatus stat = this->ReloadFromStream(id.resourceId, stream);
        this->asyncSection.Enter();
        if (stat == Success && success != nullptr)		
            success(id);
        else if (stat == Failed && failed != nullptr)	
            failed(id);
        this->asyncSection.Leave();

        // close stream
        stream->Close();
    }
    else
    {
        // if we fail to reload, just keep the old resource!
        this->asyncSection.Enter();
        if (failed != nullptr)
            failed(id);
        this->asyncSection.Leave();
        n_printf("Failed to reload resource %s\n", this->names[id.cacheInstanceId].Value());
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceLoader::SetMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
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
