//------------------------------------------------------------------------------
// resourceloader.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "resourcestreampool.h"
#include "io/ioserver.h"
#include "resourceserver.h"

using namespace IO;
namespace Resources
{

__ImplementAbstractClass(Resources::ResourceStreamPool, 'RSLO', Resources::ResourcePool);
//------------------------------------------------------------------------------
/**
*/
ResourceStreamPool::ResourceStreamPool() :
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
ResourceStreamPool::~ResourceStreamPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**

*/
void
ResourceStreamPool::Setup()
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
ResourceStreamPool::Discard()
{
    this->streamerThread->Stop();
    this->streamerThread = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceStreamPool::LoadFallbackResources()
{
    // load placeholder, don't load it async
    if (this->placeholderResourceName.IsValid())
    {
        this->placeholderResourceId = this->CreateResource(this->placeholderResourceName, "system"_atm, nullptr, nullptr, true);
    }

    // load error, don't load it async
    if (this->failResourceName.IsValid())
    {
        this->failResourceId = this->CreateResource(this->failResourceName, "system"_atm, nullptr, nullptr, true);
    }
}

//------------------------------------------------------------------------------
/**
*/
ResourceStreamPool::LoadStatus 
ResourceStreamPool::ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream)
{
    return ResourceStreamPool::Failed;
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceStreamPool::StreamMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
{
    // implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceStreamPool::Update(IndexT frameIndex)
{
    IndexT i;
    for (i = this->pendingLoads.Size() - 1; i >= 0; i--)
    {
        // get pending element
        _PendingResourceLoad& resourceLoad = this->loads[this->pendingLoads[i]];

        // load resource, get status from load function
        LoadStatus status = this->PrepareLoad(resourceLoad);
        if (status != Delay) // if the loader tells us to delay, we delay the loading to next frame
            this->pendingLoads.EraseIndex(i);
        if (status != Threaded) // if it is an immediate load, run the callbacks directly
            this->RunCallbacks(status, resourceLoad.id);
    }

    // go through pending lod streams
    this->pendingStreamQueue.DequeueAll(this->pendingStreamLods);
    for (i = this->pendingStreamLods.Size() - 1; i >= 0; i--)
    {
        const _PendingStreamLod& streamLod = this->pendingStreamLods[i];

        // skip this resource if not loaded, but keep it in the list
        this->asyncSection.Enter();
        bool resourceNotLoaded = this->states[streamLod.id.poolId] != Resource::Loaded;
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
        if (this->states[unload.resourceId.poolId] != Resource::Pending)
        {
            if (this->states[unload.resourceId.poolId] == Resource::Loaded)
            {
                // unload if loaded
                this->Unload(unload.resourceId);
                this->states[unload.resourceId.poolId] = Resource::Unloaded;
            }

            // give up the resource id
            this->DeallocObject(unload.resourceId.resourceId);
            this->resourceInstanceIndexPool.Dealloc(unload.resourceId.poolId);
            
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
ResourceStreamPool::RunCallbacks(LoadStatus status, const Resources::ResourceId id)
{
    Util::Array<_Callbacks>& cbls = this->callbacks[id.poolId];
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
ResourceStreamPool::LoadStatus
ResourceStreamPool::PrepareLoad(_PendingResourceLoad& res)
{
    LoadStatus ret = Failed;

    // in case this resource has been loaded previously
    if (this->states[res.id.poolId] == Resource::Loaded) 
        return Success;

    // if threaded, and resource is not requested to be immediate
    if (this->async && !res.immediate)
    {
        // wrap the loading process as a lambda function and pass it to the thread
        auto loadFunc = [this, &res]()
        {
            // construct stream
            Ptr<Stream> stream = IO::IoServer::Instance()->CreateStream(this->names[res.id.poolId].Value());
            stream->SetAccessMode(Stream::ReadAccess);

            // enter critical section
            if (stream->Open())
            {
                LoadStatus stat = this->LoadFromStream(res.id, res.tag, stream, res.immediate);
                this->asyncSection.Enter();
                this->RunCallbacks(stat, res.id);
                if (stat == Success)        
                    this->states[res.id.poolId] = Resource::Loaded;
                else if (stat == Failed)    
                    this->states[res.id.poolId] = Resource::Failed;
                this->asyncSection.Leave();
            }
            else
            {
                // this constitutes a failure too!
                this->asyncSection.Enter();
                this->RunCallbacks(Failed, res.id);
                this->asyncSection.Leave();
                this->states[res.id.poolId] = Resource::Failed;
                n_printf("Failed to load resource %s\n", this->names[res.id.poolId].Value());
            }
        };

        res.inflight = true;

        // add job to resource manager
        ResourceServer::Instance()->loaderThread->jobs.Enqueue(loadFunc);

        ret = Threaded;
    }
    else
    {
        // construct stream
        Ptr<Stream> stream = IoServer::Instance()->CreateStream(this->names[res.id.poolId].Value());
        stream->SetAccessMode(Stream::ReadAccess);
        if (stream->Open())
        {
            ret = this->LoadFromStream(res.id, res.tag, stream, res.immediate);
            this->asyncSection.Enter();
            this->RunCallbacks(ret, res.id);
            if (ret == Success)
                this->states[res.id.poolId] = Resource::Loaded;
            else if (ret == Failed)
                this->states[res.id.poolId] = Resource::Failed;
            this->asyncSection.Leave();
        }
        else
        {
            this->asyncSection.Enter();
            this->RunCallbacks(Failed, res.id);
            this->states[res.id.poolId] = Resource::Failed;
            this->asyncSection.Leave();
            ret = Failed;
            n_printf("Failed to load resource %s\n", this->names[res.id.poolId].Value());
        }
    }   
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceId
Resources::ResourceStreamPool::CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
{
    // this assert should maybe be removed in favor of putting things on a queue if called from another thread
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);

    Resources::ResourceId ret;
    ResourceUnknownId resourceId; // this is the id of the resource 
    IndexT i = this->ids.FindIndex(res);

    if (i == InvalidIndex)
    {
        // allocate new object (AllocObject is implemented using the __ImplementResourceAllocator macro, or in a specialized allocator)
        resourceId = this->AllocObject();

        // allocate new index for the allocator
        Ids::Id32 instanceId = this->resourceInstanceIndexPool.Alloc(); // this is the ID of the container

        // create new resource id, if need be, grow the container list
        if (instanceId >= (uint)this->names.Size())
        {
            this->usage.Resize(this->usage.Size() + ResourceIndexGrow);
            this->callbacks.Resize(this->callbacks.Size() + ResourceIndexGrow);
            this->names.Resize(this->names.Size() + ResourceIndexGrow);
            this->tags.Resize(this->tags.Size() + ResourceIndexGrow);
            this->states.Resize(this->states.Size() + ResourceIndexGrow);
            this->loads.Resize(this->states.Size() + ResourceIndexGrow);
        }

        // add the resource name to the resource id
        this->names[instanceId] = res;
        this->usage[instanceId] = 1;
        this->tags[instanceId] = tag;
        this->states[instanceId] = Resource::Pending;

        // also add as pending resource
        ret.poolId = instanceId;
        ret.poolIndex = this->uniqueId;
        ret.resourceId = resourceId.id24;
        ret.resourceType = resourceId.id8;

        _PendingResourceLoad pending;
        pending.id = ret;
        pending.tag = tag;
        pending.inflight = false;
        pending.immediate = immediate;
        this->loads[instanceId] = pending;

        // add mapping between resource name and resource being loaded
        this->ids.Add(res, ret);

        if (immediate)
        {
            LoadStatus status = this->PrepareLoad(pending);
            if (status == Success)
            {
                if (success != nullptr)
                    success(ret);
            }
            else if (status == Failed)
            {
                // change return resource id to be fail resource
                ret.resourceId = this->failResourceId.resourceId;
                if (failed != nullptr)
                    failed(ret);
            }
        }
        else
        {
            this->pendingLoads.Append(instanceId);
            if (success != nullptr || failed != nullptr)
            {
                // we need not worry about the thread, since this resource is new
                this->callbacks[instanceId].Append({ ret, success, failed });
            }

            // set to placeholder while waiting
            ret.resourceId = placeholderResourceId.resourceId;
        }
    }
    else // this means the resource container is already created, and it may or may not be pending
    {
        // get id of resource
        ret = this->ids.ValueAtIndex(i);

        // bump usage
        this->usage[ret.poolId]++;

        // start the async section, the loader might change the resource state
        this->asyncSection.Enter();
        
        // if the resource has been loaded (through a previous Update), just call the success callback
        const Resource::State state = this->states[ret.poolId];
        if (state == Resource::Loaded && success != nullptr)
        {
            // if loaded and not immediate, run callback, otherwise just return id
            success(ret);
        }
        else if (state == Resource::Failed && !immediate && failed != nullptr)
        {
            // if failed and not immediate, run callback, otherwise just return id
            failed(ret);
        }
        else if (state == Resource::Pending)
        {
            // this resource should now be in the pending list
            n_assert(i != InvalidIndex);

            // pending resource may not be in-flight in thread
            _PendingResourceLoad& pend = this->loads[ret.poolId];
            if (!pend.inflight)
            {
                // flip the immediate flag, this is in case we decide to perform a later load using immediate override
                pend.immediate = pend.immediate || immediate;
            }

            // since we are pending and inside the async section, it means the resource is not loaded yet, which means its safe to add the callback
            this->callbacks[ret.poolId].Append({ ret, success, failed });

            // set to placeholder while waiting
            ret.resourceId = placeholderResourceId.resourceId;
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
Resources::ResourceStreamPool::DiscardResource(const Resources::ResourceId id)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    if (id != this->placeholderResourceId && id != this->failResourceId)
    {
        ResourcePool::DiscardResource(id);

        // if usage reaches 0, add it to the list of pending unloads
        if (this->usage[id.poolId] == 0)
        {
            if (this->async)
            {
                // add pending unload, it will be unloaded once loaded
                this->pendingUnloads.Append({ id });
            
            }
            else
            {
                this->Unload(id);
                this->DeallocObject(id.AllocId());
            }
            this->resourceInstanceIndexPool.Dealloc(id.poolId);
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
ResourceStreamPool::DiscardByTag(const Util::StringAtom& tag)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    IndexT i;
    for (i = 0; i < this->tags.Size(); i++)
    {
        if (this->tags[i] == tag)
        {
            // add pending unload, it will be unloaded once loaded
            this->pendingUnloads.Append({ this->ids[this->names[i]] });
            this->tags[i] = "";
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceStreamPool::ReloadResource(const Resources::ResourceName& res, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed)
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
        Ptr<Stream> stream = ioserver->CreateStream(this->names[ret.poolId].Value());
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
            n_printf("Failed to reload resource %s\n", this->names[ret.poolId].Value());
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
ResourceStreamPool::ReloadResource(const Resources::ResourceId& id, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed)
{
    n_assert(Threading::Thread::GetMyThreadId() == this->creatorThread);
    n_assert_fmt(id != Resources::ResourceId::Invalid(), "Resource %d is not loaded, it has to be before it can be reloaded", id.HashCode());

    // copy the reference
    IoServer* ioserver = IoServer::Instance();

    // construct stream
    Ptr<Stream> stream = ioserver->CreateStream(this->names[id.poolId].Value());
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
        n_printf("Failed to reload resource %s\n", this->names[id.poolId].Value());
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceStreamPool::SetMaxLOD(const Resources::ResourceId& id, const float lod, bool immediate)
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
