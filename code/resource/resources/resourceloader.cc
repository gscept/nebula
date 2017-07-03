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
ResourceLoader::ResourceLoader()
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
				if (element.success != nullptr) element.success(res);
				res->state = Resource::Loaded;

				this->usage.Add(res->resourceId, 1);
				this->loaded.Add(res->resourceId, res);
				this->tags.Add(res->resourceId, element.tag);
			}
			else if (status == Failed)
			{
				if (element.failed != nullptr) element.failed(res);
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
					if (resCpy.success != nullptr) resCpy.success(res);
					res->state = Resource::Loaded;

					// enter async section, and update resource loader status
					this->loaded.Add(res->resourceId, res);
					this->tags.Add(res->resourceId, resCpy.tag);
				}
				else if (stat == Failed)
				{
					if (resCpy.failed != nullptr) resCpy.failed(resCpy.res);
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
				if (resCpy.failed != nullptr) resCpy.failed(resCpy.res);
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
void
ResourceLoader::DiscardByTag(const Util::StringAtom& tag)
{
	IndexT i;
	for (i = 0; i < loaded.Size(); i++)
	{
		const Util::StringAtom& tag = this->tags.ValueAtIndex(i);
		if (tag == tag)
		{
			this->Unload(this->loaded.ValueAtIndex(i));
			this->loaded.EraseAtIndex(i);
			this->tags.EraseAtIndex(i);
		}		
	}
}

} // namespace Resources