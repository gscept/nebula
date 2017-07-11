//------------------------------------------------------------------------------
// resourceloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourceloader.h"
#include "io/ioserver.h"
#include "jobs/job.h"
#include "jobs/jobfunccontext.h"
#include "resourcemanager.h"

using namespace IO;
namespace Resources
{

__ImplementAbstractClass(Resources::ResourceLoader, 'RELO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
ResourceLoader::ResourceLoader() :
	resourceInstanceIndexPool(0xFFFFFFFF),
	resourceIndexPool(0x00FFFFFF)
{
	// empty
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
	this->resourceClass = nullptr;
	this->placeholderResourceId = "";
	this->errorResourceId = "";
	this->uniqueResourceId = 0;

	// create job port in case we want the loader to be executed asynchronously
	this->jobport = Jobs::JobPort::Create();
	this->jobport->Setup();
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Discard()
{
	this->jobport->Discard();
	this->jobport = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Update(IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->pending.Size(); i++)
	{
		_PendingResource& element = this->pending.ValueAtIndex(i);

		// this means the pending resource is being loaded in a thread
		if (element.inflight) continue;
		const Ptr<Resources::Resource>& res = element.res;

		// load resource, get status from load function
		LoadStatus status = this->PrepareLoad(element);
		if (status != Delay)
		{
			// enter critical section
			this->criticalSection.Enter();
			if (status == Threaded) continue;
			else if (status == Success)
			{
				if (element.success != nullptr) element.success(element.id);
				res->state = Resource::Loaded;
			}
			else if (status == Failed)
			{
				if (element.failed != nullptr) element.failed(element.id);
				res->state = Resource::Failed;
			}
			this->pending.EraseAtIndex(i--);
			this->criticalSection.Leave();
		}		
	}
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::LoadStatus
ResourceLoader::PrepareLoad(_PendingResource& res)
{
	LoadStatus ret = Failed;

	// if threaded, and resource is not requested to be immediate
	if (this->async && !res.immediate)
	{
		// copy the reference
		_PendingResource resCpy = res;
		IoServer* ioserver = IoServer::Instance();

		// add to set of asynchronously loading resources
		this->asyncSection.Enter();
		
		// wrap the loading process as a lambda function and pass it to the thread
		auto loadFunc = [this, resCpy, ioserver]()
		{
			// construct stream
			const Ptr<Resources::Resource>& res = resCpy.res;
			Ptr<Stream> stream = ioserver->CreateStream(resCpy.res->resourceId.Value());
			stream->SetAccessMode(Stream::ReadAccess);
			if (stream->Open())
			{
				// enter critical section, this is quite big
				this->asyncSection.Enter();
				LoadStatus stat = this->Load(resCpy.res, stream);
				if (stat == Success)
				{
					if (resCpy.success != nullptr) resCpy.success(resCpy.id);
					res->state = Resource::Loaded;
				}
				else if (stat == Failed)
				{
					if (resCpy.failed != nullptr) resCpy.failed(resCpy.id);
					res->state = Resource::Failed;
				}

				// no matter the result, we have to remove this resource from the pending list
				this->pending.Erase(res->resourceId);
				this->asyncSection.Leave();

				// close stream
				stream->Close();
			}
			else
			{
				// this constitutes a failure too!
				if (resCpy.failed != nullptr) resCpy.failed(resCpy.id);
				res->state = Resource::Failed;
			}
		};

		// flag resource as being in-flight
		res.inflight = true;
		res.loadFunc = loadFunc;

		// add job to resource manager
		ResourceManager::Instance()->loaderThread->jobs.Enqueue(loadFunc);
		this->asyncSection.Leave();

		ret = Threaded;
	}
	else
	{
		// construct stream
		Ptr<Stream> stream = IoServer::Instance()->CreateStream(res.res->resourceId.Value());
		if (stream->Open())
		{
			ret = this->Load(res.res, stream);
			stream->Close();
		}
	}	
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
Core::Id
Resources::ResourceLoader::CreateResource(const ResourceId& res, const Util::StringAtom& tag, std::function<void(const Core::Id&)> success, std::function<void(const Core::Id&)> failed, bool immediate)
{
	n_assert(this->resourceClass != nullptr);
	n_assert(this->resourceClass->IsDerivedFrom(Resource::RTTI));

	Core::Id ret;
	uint32_t resourceId; // this is the id of the resource
	uint32_t instanceId = this->resourceInstanceIndexPool.Alloc(); // this is the ID of the container
	IndexT i = this->ids.FindIndex(res);

	if (i == InvalidIndex)
	{
		// create new resource id, if need be, grow the container list
		resourceId = this->resourceIndexPool.Alloc();
		if (resourceId >= (uint32_t)this->containers.Size() )
		{
			this->containers.Resize(this->containers.Size() + 512);
			this->usage.Resize(this->usage.Size() + 512);
		}

		// grab container
		ResourceContainer& container = this->containers[resourceId];
		container.error = this->errorResource;
		container.placeholder = this->placeholderResource;
		container.resource = this->resourceClass->Create();
		container.resource->resourceId = res;
		container.resource->tag = tag;

		// add the resource name to the resource id
		this->ids.Add(res, resourceId);
		this->usage[resourceId] = 1;

		// also add as pending resource
		this->pending.Add(res, { ret, container.resource, success, failed, tag, false, nullptr, immediate });
	}
	else // this means we have created this resource previously
	{
		// bump id and grab container, both Dictionaries should have the same indexing
		resourceId = this->ids.ValueAtIndex(i);

		// get ids
		this->usage[resourceId]++;
		const ResourceContainer& container = this->containers[resourceId];
		
		// if resource is loaded, call success callback
		if (container.resource->state == Resource::Loaded)
		{
			if (success != nullptr) success(ret);
		}
	}

	ret = Core::Id::MakeId(instanceId, Core::Id::MakeBigTiny(resourceId, this->uniqueId));
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
Resources::ResourceLoader::DiscardResource(const Core::Id& res)
{
	//static_assert(std::is_base_of<Resource, RESOURCE_TYPE>::value, "Type is not a subclass of Resources::Resource");
	n_assert(this->resourceClass != nullptr);
	n_assert(this->resourceClass->IsDerivedFrom(Resource::RTTI));

	// the id of the usage and container respectively is in the big part of the low bits
	uint32_t instanceId = Core::Id::GetHigh(res);
	uint32_t resourceId = Core::Id::GetBig(Core::Id::GetLow(res));

	// dealloc instance id and reduce usage
	this->resourceInstanceIndexPool.Dealloc(instanceId);
	this->usage[resourceId]--;
	const ResourceContainer& container = this->containers[resourceId];
	n_assert_fmt(!container.resource->tag.IsValid(), "Resource with tag can not be individually deleted");

	// if usage reaches 0, either discard pending resource or unload it
	if (this->usage[resourceId] == 0)
	{
		if (container.resource->state == Resource::Pending)
		{
			this->criticalSection.Enter();
			IndexT j = this->pending.FindIndex(container.resource->resourceId);
			n_assert(j != InvalidIndex);
			const _PendingResource& res = this->pending.ValueAtIndex(j);

			// hmm, resource is being loaded by a thread
			if (res.inflight)
			{
				// wait for resource loader thread to finish, then delete it
				// this is very unfortunate, but we cannot abort the thread job safely without a sync point
				n_assert(res.loadFunc != nullptr);
				ResourceManager::Instance()->loaderThread->Wait();
				this->Unload(container.resource);
			}
			else
			{
				// call failed function to indicate the load was aborted
				if (res.failed != nullptr) res.failed(res.id);

				// just remove from pending, easy peasy
				this->pending.EraseAtIndex(j);
			}
			this->criticalSection.Leave();
		}
		else
		{

			// unload the resource in question
			this->Unload(container.resource);

			// just free the index from the array, but do not actually shrink the array
			this->resourceIndexPool.Dealloc(resourceId);
			container.resource->state = Resource::Unloaded;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::DiscardByTag(const Util::StringAtom& tag)
{
	IndexT i;
	for (i = 0; i < containers.Size(); i++)
	{
		ResourceContainer& container = containers[i];
		if (container.resource->tag == tag)
		{
			if (container.resource->state == Resource::Loaded)
			{
				const uint32_t id = this->ids[tag];
				this->Unload(container.resource);
				this->resourceIndexPool.Dealloc(id);
				container.resource->state = Resource::Unloaded;
			}
			else
			{
				this->criticalSection.Enter();
				IndexT j = this->pending.FindIndex(container.resource->resourceId);
				n_assert(j != InvalidIndex);
				const _PendingResource& res = this->pending.ValueAtIndex(j);

				// hmm, resource is being loaded by a thread
				if (res.inflight)
				{
					// wait for resource loader thread to finish, then delete it
					// this is very unfortunate, but we cannot abort the thread job safely without a sync point
					n_assert(res.loadFunc != nullptr);
					ResourceManager::Instance()->loaderThread->Wait();
					this->Unload(container.resource);
				}
				else
				{
					// call failed function to indicate the load was aborted
					if (res.failed != nullptr) res.failed(res.id);

					// just remove from pending, easy peasy
					this->pending.EraseAtIndex(j);
				}
				this->criticalSection.Leave();
			}
		}
	}
}

} // namespace Resources