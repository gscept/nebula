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
	pendingLoadPool(1024),
	pendingStreamLods(1024)
{
	// maybe this is arrogant, just 1024 pending resources (actual resources that is) per loader?
	this->pendingLoads.Resize(1024);
	this->pendingStreamLods.Resize(1024);
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
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceStreamPool::Discard()
{
	// empty
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
ResourceStreamPool::UpdateLOD(const Resources::ResourceId& id, const IndexT lod, bool immediate)
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
	for (i = 0; i < this->pendingLoads.Size(); i++)
	{
		// get pending element
		_PendingResourceLoad& resourceLoad = this->pendingLoads[i];

		// skip unused elements
		if (resourceLoad.id.poolId == Ids::InvalidId24) continue;

		// save id here, because we might modify the id to know the pending resource should be finished
		Resources::ResourceId id = resourceLoad.id;

		// if the element pool index is invalidid8, it means it's been processed
		if (resourceLoad.id.poolIndex == Ids::InvalidId8)
		{
			// callbacks are called from the thread, we only have to clear them
			this->callbacks[resourceLoad.id.poolId].Clear();
			this->pendingLoadPool.Dealloc(resourceLoad.id.poolId);
			this->pendingLoadMap.Erase(this->names[resourceLoad.id.poolId]);
			resourceLoad.id.poolId = Ids::InvalidId24;
			continue;
		}

		// if the resource is inflight, it's still going to be in the list, but we skip it
		if (resourceLoad.inflight) continue;

		// load resource, get status from load function
		LoadStatus status = this->PrepareLoad(resourceLoad);
		if (status != Delay)
		{
			// if not threaded, then run callbacks, clear callbacks and pending resource
			if (status != Threaded)
			{
				// immediate load, run callbacks on status, erase callbacks, and return id of pending element
				this->RunCallbacks(status, id);
				this->callbacks[resourceLoad.id.poolId].Clear();
				this->pendingLoadPool.Dealloc(resourceLoad.id.poolId);
				this->pendingLoadMap.Erase(this->names[resourceLoad.id.poolId]);
				resourceLoad.id.poolId = Ids::InvalidId24;
			}
		}		
	}

	// go through pending lod streams
	for (i = 0; i < this->pendingStreamLods.Size(); i++)
	{
		_PendingStreamLod& streamLod = this->pendingStreamLods[i];

		// skip unused elements
		if (streamLod.id.poolId == Ids::InvalidId24) continue;

		// skip this resource if not loaded
		if (this->GetState(streamLod.id) != Resource::Loaded)
			continue;

		// if the pool index is invalid, it means we have processed this entry
		if (streamLod.id.poolIndex == Ids::InvalidId8)
		{
			// callbacks are called from the thread, we only have to clear them
			this->pendingStreamPool.Dealloc(streamLod.id.poolId);
			streamLod.id.poolId = Ids::InvalidId24;
			continue;
		}

		// if async is supported, put the stream job on a thread!
		if (this->async && !streamLod.immediate)
		{
			auto streamFunc = [this, &streamLod]()
			{
				this->UpdateLOD(streamLod.id, streamLod.lod, streamLod.immediate);
				streamLod.id.poolIndex = Ids::InvalidId8;
			};
			this->AddThreadJob(streamFunc);
		}
		else
		{
			// load immediately
			this->UpdateLOD(streamLod.id, streamLod.lod, streamLod.immediate);
			streamLod.id.poolIndex = Ids::InvalidId8;
		}
	}

	// go through pending unloads
	for (i = 0; i < this->pendingUnloads.Size(); i++)
	{
		const _PendingResourceUnload& unload = this->pendingUnloads[i];
		if (this->states[unload.resourceId.poolId] != Resource::Pending)
		{
			if (this->states[unload.resourceId.poolId] == Resource::Loaded)
			{
				// unload if loaded
				this->Unload(unload.resourceId.resourceId);
				this->states[unload.resourceId.poolId] = Resource::Unloaded;
			}

			// give up the resource id
			this->DeallocObject(unload.resourceId.resourceId);
			this->resourceInstanceIndexPool.Dealloc(unload.resourceId.poolId);
			
			// remove pending unload if not Pending or Loaded (so explicitly Unloaded or Failed)
			this->pendingUnloads.EraseIndex(i--);
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
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceStreamPool::AddThreadJob(std::function<void()> func)
{
	ResourceServer::Instance()->loaderThread->jobs.Enqueue(func);
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

			// mark that we are done with this resource so it may be cleared later
			res.id.poolIndex = Ids::InvalidId8;
		};

		// flag resource as being in-flight
		res.inflight = true;

		// add job to resource manager
		this->AddThreadJob(loadFunc);

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
		}
		else
		{
			ret = Failed;
			n_printf("Failed to load resource %s\n", this->names[res.id.poolId].Value());
		}

		// mark that we are done with this resource so it may be cleared later
		res.id.poolIndex = Ids::InvalidId8;
	}	
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceId
Resources::ResourceStreamPool::CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate)
{
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
		
		// add mapping between resource name and resource being loaded
		this->ids.Add(res, ret);

		Ids::Id32 pendingId = this->pendingLoadPool.Alloc();
		_PendingResourceLoad& pending = this->pendingLoads[pendingId];
		pending.id = ret;
		pending.tag = tag;
		pending.inflight = false;
		pending.immediate = immediate;

		if (immediate)
		{
			LoadStatus status = this->PrepareLoad(pending);
			this->pendingLoadPool.Dealloc(pendingId);
			pending = _PendingResourceLoad();
			if (status == Success)
			{
				if (success != nullptr) 
					success(ret);
				this->states[instanceId] = Resource::Loaded;
			}
			else if (status == Failed)
			{
				// change return resource id to be fail resource
				ret.resourceId = this->failResourceId.resourceId;

				if (failed != nullptr)
				{
					failed(ret);
				}
				this->states[instanceId] = Resource::Failed;
			}
		}
		else
		{
			if (success != nullptr || failed != nullptr)
			{
				// we need not worry about the thread, since this resource is new
				this->callbacks[instanceId].Append({ ret, success, failed });
			}

			this->pendingLoadMap.Add(res, pendingId);

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
		if (state == Resource::Loaded && !immediate)
		{
			// if loaded and not immediate, run callback, otherwise just return id
			success(ret);
		}
		else if (state == Resource::Failed && !immediate)
		{
			// if failed and not immediate, run callback, otherwise just return id
			failed(ret);
		}
		else if (state == Resource::Pending)
		{
			// this resource should now be in the pending list
			IndexT i = this->pendingLoadMap.FindIndex(res);
			n_assert(i != InvalidIndex);

			// pending resource may not be in-flight in thread
			_PendingResourceLoad& pend = this->pendingLoads[this->pendingLoadMap.ValueAtIndex(i)];
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
			if (stat == Success && success)		success(ret);
			else if (stat == Failed && success)	failed(ret);
			this->asyncSection.Leave();

			// close stream
			stream->Close();
		}
		else
		{
			// if we fail to reload, just keep the old resource!
			this->asyncSection.Enter();
			if (failed) failed(ret);
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
		if (stat == Success)		success(id);
		else if (stat == Failed)	failed(id);
		this->asyncSection.Leave();

		// close stream
		stream->Close();
	}
	else
	{
		// if we fail to reload, just keep the old resource!
		this->asyncSection.Enter();
		failed(id);
		this->asyncSection.Leave();
		n_printf("Failed to reload resource %s\n", this->names[id.poolId].Value());
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ResourceStreamPool::UpdateResourceLOD(const Resources::ResourceId& id, const IndexT lod, bool immediate)
{
	const Resource::State state = this->states[id.poolId];

	Ids::Id32 pendingId = this->pendingStreamPool.Alloc();
	_PendingStreamLod& pending = this->pendingStreamLods[pendingId];
	pending.id = id;
	pending.lod = lod;
	pending.immediate = immediate;
}

} // namespace Resources
