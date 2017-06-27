//------------------------------------------------------------------------------
// resourceloader.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "resourceloader.h"
#include "io/ioserver.h"
#include "jobs/job.h"

using namespace IO;
namespace Resources
{

__ImplementClass(Resources::ResourceLoader, 'RELO', Core::RefCounted);
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
	this->resourceClass = RefCounted::RTTI;
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
		const Ptr<Resources::Resource>& res = element.res;

		// load resource, get status from load function
		LoadStatus status = this->PrepareLoad(element);
		if (status != Delay)
		{
			if (status == Success)
			{
				element.success(res);
				this->usage.Add(res->resourceId, 1);
				this->loaded.Add(res->resourceId, res);
				this->tags.Add(res->resourceId, element.tag);
			}
			else if (status == Failed)	element.failed(res);
			/// callback for threaded resources happens in the thread
			this->pending.EraseAtIndex(i--);
		}		
	}
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::LoadStatus
ResourceLoader::PrepareLoad(_PendingResource& res)
{
	// if threaded
	if (this->async)
	{
		// copy the reference
		_PendingResource resCpy = res;

		// wrap the loading process as a lambda function and pass it to the thread
		auto loadFunc = [this, resCpy]()
		{
			// construct stream
			Ptr<Stream> stream = IoServer::Instance()->CreateStream(resCpy.res->resourceId.Value());
			if (stream->Open())
			{
				LoadStatus stat = this->Load(resCpy.res, stream);
				if (stat == Success)		resCpy.success(resCpy.res);
				else if (stat == Failed)	resCpy.failed(resCpy.res);
				stream->Close();
			}
			else
			{
				// this constitutes a failure too!
				resCpy.failed(resCpy.res);
			}
		};

		// create job
		Ptr<Jobs::Job> loaderJob = Jobs::Job::Create();
		Jobs::JobFuncDesc func(loadFunc);

		// setup job, no need for uniforms, inputs or outputs!
		loaderJob->Setup(nullptr, nullptr, nullptr, func);
		this->jobport->PushJob(loaderJob);
		return Threaded;
	}
	else
	{
		// construct stream
		Ptr<Stream> stream = IoServer::Instance()->CreateStream(res.res->resourceId.Value());
		if (stream->Open())
		{
			return this->Load(res.res, stream);
			stream->Close();
		}
	}	
	return Failed;
}

//------------------------------------------------------------------------------
/**
*/
ResourceLoader::LoadStatus
ResourceLoader::Load(const Ptr<Resource>& res, const Ptr<IO::Stream>& stream)
{

}

//------------------------------------------------------------------------------
/**
*/
void
ResourceLoader::Unload(const Ptr<Resource>& res)
{
	// override in subclass to perform actual resource unload
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
			this->usage.EraseAtIndex(i);
			this->tags.EraseAtIndex(i);
		}		
	}
}

} // namespace Resources