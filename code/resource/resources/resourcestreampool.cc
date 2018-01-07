//------------------------------------------------------------------------------
// resourceloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourcestreampool.h"
#include "io/ioserver.h"
#include "jobs/job.h"
#include "jobs/jobfunccontext.h"
#include "resourcemanager.h"

using namespace IO;
namespace Resources
{

__ImplementAbstractClass(Resources::ResourceStreamPool, 'RSLO', Resources::ResourcePool);
//------------------------------------------------------------------------------
/**
*/
ResourceStreamPool::ResourceStreamPool() :
	pendingLoadPool(1024)
{
	// maybe this is arrogant, just 1024 pending resources (actual resources that is) per loader?
	this->pendingLoads.Resize(1024);
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
	this->placeholderResourceId = "";
	this->errorResourceId = "";
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
ResourceStreamPool::Update(IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->pendingLoads.Size(); i++)
	{
		// get pending element
		_PendingResourceLoad& element = this->pendingLoads[i];

		// skip unused elements
		if (element.pid == Ids::InvalidId32) continue;

		Ids::Id24 resourceId = element.id.id24;

		// if the element has an invalid id, it means a thread has eaten it
		if (element.id.id8 == Ids::InvalidId8)
		{
			// callbacks are called from the thread, we only have to clear them
			this->callbacks[resourceId].Clear();
			this->pendingLoadPool.Dealloc(element.pid);
			this->pendingLoadMap.Erase(this->names[element.res]);
			element.pid = Ids::InvalidId32;
			continue;
		}

		// if the resource is inflight, it's still going to be in the list, but we skip it
		if (element.inflight) continue;

		// load resource, get status from load function
		LoadStatus status = this->PrepareLoad(element);
		if (status != Delay)
		{
			// if not threaded, then run callbacks, clear callbacks and pending resource
			if (status != Threaded)
			{
				// immediate load, run callbacks on status, erase callbacks, and return id of pending element
				this->RunCallbacks(status, element.id);
				this->callbacks[resourceId].Clear();
				this->pendingLoadPool.Dealloc(element.pid);
				this->pendingLoadMap.Erase(this->names[element.res]);
				element.pid = Ids::InvalidId32;
			}
		}		
	}

	// go through pending unloads
	for (i = 0; i < this->pendingUnloads.Size(); i++)
	{
		const _PendingResourceUnload& unload = this->pendingUnloads[i];
		if (this->states[unload.resourceId] != Resource::Pending)
		{
			if (this->states[unload.resourceId] == Resource::Loaded)
			{
				// unload if loaded
				this->Unload(unload.resourceId);
				this->states[unload.resourceId] = Resource::Unloaded;
			}

			// give up the resource id
			this->DeallocObject(unload.resourceId);
			
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
	Util::Array<_Callbacks>& cbls = this->callbacks[id.id24];
	IndexT i;
	for (i = 0; i < cbls.Size(); i++)
	{
		const _Callbacks& cbl = cbls[i];
		if (status == Success && cbl.success != nullptr)	cbl.success(id);
		else if (status == Failed && cbl.failed != nullptr) cbl.failed(id);
	}
}

//------------------------------------------------------------------------------
/**
*/
ResourceStreamPool::LoadStatus
ResourceStreamPool::PrepareLoad(_PendingResourceLoad& res)
{
	LoadStatus ret = Failed;

	// in case this resource has been loaded previously
	if (this->states[res.res] == Resource::Loaded) return Success;

	// if threaded, and resource is not requested to be immediate
	if (this->async && !res.immediate)
	{
		// copy the reference
		IoServer* ioserver = IoServer::Instance();
		
		// wrap the loading process as a lambda function and pass it to the thread
		auto loadFunc = [this, &res, ioserver]()
		{
			// construct stream
			Ptr<Stream> stream = ioserver->CreateStream(this->names[res.res].Value());
			stream->SetAccessMode(Stream::ReadAccess);

			// enter critical section
			if (stream->Open())
			{
				LoadStatus stat = this->LoadFromStream(res.res, res.tag, stream);
				this->asyncSection.Enter();
				this->RunCallbacks(stat, res.id);
				if (stat == Success)		this->states[res.res] = Resource::Loaded;
				else if (stat == Failed)	this->states[res.res] = Resource::Failed;
				this->asyncSection.Leave();

				// close stream
				stream->Close();
			}
			else
			{
				// this constitutes a failure too!
				this->asyncSection.Enter();
				this->RunCallbacks(Failed, res.id);
				this->asyncSection.Leave();
				this->states[res.res] = Resource::Failed;
			}

			// mark that we are done with this resource so it may be cleared later
			res.id.id8 = Ids::InvalidId8;
		};

		// flag resource as being in-flight
		res.inflight = true;
		res.loadFunc = loadFunc;

		// add job to resource manager
		ResourceManager::Instance()->loaderThread->jobs.Enqueue(loadFunc);

		ret = Threaded;
	}
	else
	{
		// construct stream
		Ptr<Stream> stream = IoServer::Instance()->CreateStream(this->names[res.res].Value());
		if (stream->Open())
		{
			ret = this->LoadFromStream(res.res, res.tag, stream);
			stream->Close();
		}
		else
		{
			ret = Failed;
		}

		// mark that we are done with this resource so it may be cleared later
		res.id.id8 = Ids::InvalidId8;
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
	Ids::Id32 instanceId = this->resourceInstanceIndexPool.Alloc(); // this is the ID of the container
	Ids::Id24 resourceId; // this is the id of the resource	
	IndexT i = this->ids.FindIndex(res);

	if (i == InvalidIndex)
	{
		// allocate new object (AllocObject is implemented using the __ImplementResourceAllocator macro)
		resourceId = this->AllocObject();

		// create new resource id, if need be, grow the container list
		if (resourceId >= (uint32_t)this->names.Size())
		{
			this->usage.Resize(this->usage.Size() + ResourceIndexGrow);
			this->callbacks.Resize(this->callbacks.Size() + ResourceIndexGrow);
			this->names.Resize(this->names.Size() + ResourceIndexGrow);
			this->tags.Resize(this->tags.Size() + ResourceIndexGrow);
			this->states.Resize(this->states.Size() + ResourceIndexGrow);
		}

		// add the resource name to the resource id
		this->ids.Add(res, resourceId);
		this->names[resourceId] = res;
		this->usage[resourceId] = 1;
		this->tags[resourceId] = tag;
		this->states[resourceId] = Resource::Pending;

		if (success != nullptr || failed != nullptr)
		{
			// we need not worry about the thread, since this resource is new
			this->callbacks[resourceId].Append({ instanceId, success, failed });
		}		

		// also add as pending resource
		ret.id32 = instanceId;
		ret.id24 = resourceId;
		ret.id8 = this->uniqueId;

		Ids::Id32 pendingId = this->pendingLoadPool.Alloc();
		_PendingResourceLoad& pending = this->pendingLoads[pendingId];
		pending.id = ret;
		pending.pid = pendingId;
		pending.res = resourceId;
		pending.tag = tag;
		pending.inflight = false;
		pending.immediate = immediate;
		pending.loadFunc = nullptr;
		this->pendingLoadMap.Add(res, pendingId);
	}
	else // this means the resource container is already created, and it may or may not be pending
	{
		// get id of resource
		resourceId = this->ids.ValueAtIndex(i);
		ret.id32 = instanceId;
		ret.id24 = resourceId;
		ret.id8 = this->uniqueId;

		// bump usage
		this->usage[resourceId]++;

		// only do this part if we have callbacks
		if (success != nullptr || failed != nullptr)
		{
			// start the async section, the loader might change the resource state
			this->asyncSection.Enter();
		
			// if the resource has been loaded (through a previous Update), just call the success callback
			const Resource::State state = this->states[resourceId];
			if (state == Resource::Loaded)
			{
				if (success != nullptr) success(ret);
			}
			else if (state == Resource::Failed)
			{
				if (failed != nullptr) failed(ret);
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
				this->callbacks[resourceId].Append({ instanceId, success, failed });
			}

			// leave async section
			this->asyncSection.Leave();
		}
	}

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
Resources::ResourceStreamPool::DiscardResource(const Resources::ResourceId id)
{
	ResourcePool::DiscardResource(id);

	// if usage reaches 0, add it to the list of pending unloads
	if (this->usage[id.id24] == 0)
	{
		// add pending unload, it will be unloaded once loaded
		this->pendingUnloads.Append({ id.id24 });
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceStreamPool::DiscardByTag(const Util::StringAtom& tag)
{
	IndexT i;
	for (i = 0; i < tags.Size(); i++)
	{
		if (tags[i] == tag)
		{
			// add pending unload, it will be unloaded once loaded
			this->pendingUnloads.Append({ (Ids::Id24)i });
			this->tags[i] = "";
		}
	}
}

} // namespace Resources